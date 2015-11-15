/*-----------------------------------------------------------------------------------------------------------------
  This program calculates based on a set of transactions, the amount of sales per salerep and per territory.
  A salerep has only one territory, but a territory can have multiple salereps.
  All amount are long.
  Every ids in the system should be valid as input (no need for validation).
  
  There is one file for the transactions as input.
  File format:
   <trxid>,<salerepid>,<transaction type>,<amount>

 There is one file for the salereps as input/output. 
 The file contains pre-existing amount per salereps which are running totals. 
 The transactions must be applied to the starting amount per salerep.
 File format:
   <salerepid>,<territoryid>,<amount>

 There is one file for the territory as output. Amount for territory starts at 0.
 File format:
   <territoryid>,<amount>

 TRANSACTION PROCESSING

    Each transaction has different rule in term of calculating the % of the transaction's amount which must be 
	attributed to the salerep vs the territory
             Territory    Salerep     Comment
			         %          %
   SALE  :         100        100     Added
   VALUEADDED:     100        110     Added. Value added goods and services have very high marging.
   CREDIT:         100        100     Substracted
   CANCEL:         100        125     Substracted. It is the duty of a salerep to minimize invoice cancellation as they may have legal incidence
   PROMO:          100          0     Substracted. Promo are given at the corporate level, therefore the salerep quota are not affected by it
   DISCOUNT        100        110     Substracted. To minimize the likelyhood of a salerep giving discount
   INTER-TERRITORY   0         75     Added. Inter-territory sales are not accounted toward the territory total sales as the saler wil have it as a SALE. 
   
   as an example: 
   if a transaction.type == TRANSACTION.SALE with an amount of 200, both the salerep and the 
   territory will have 200 (100%) added to their amount.
   if a transaction.type == TRANSACTION.PROMO with an amount of 200, 200 will be substracted 
   from the territory (100%) but 0 to the salerep (0%)   
   if a transaction.type == TRANSACTION.INTER-TERRITORY with an amount of 200, 0 will be added 
   from the territory (0% of 200) but 150 to the salerep (75% of 200)   
   
----------------------------------------------------------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


enum TRANSACTION     
{
    SALE            = 1,      
	VALUEADDED      = 2,
    CREDIT          = 3,
    CANCEL          = 4,
    PROMO           = 5,
    DISCOUNT        = 6,
    INTER_TERRITORY = 7	
};

struct transaction {
    int trxid;
    int salerepid;
    enum TRANSACTION type;  
    long amount;
};

struct territory {
    int territoryid;
    long amount;
};

struct salerep {
    int salerepid;
    int territoryid;
    long amount;
	long filepos; //position in the input file where this salerep is located to update it at the end of processing
};


struct salerep * salereps;
struct territory * territories;
//struct transaction * transactions;


int structCount = 0;


/*------------------------------------------------------------------------------------------------------------------*/
/* function to compare salerep for sorting, sort by amount  														*/
int comparefunction (const void * a, const void * b)
{
   struct salerep * aRep =  (struct salerep *) a;
   struct salerep * bRep =  (struct salerep *) b;
   return ( aRep->amount - bRep->amount );
}

/*------------------------------------------------------------------------------------------------------------------*/
/* initialized a new salerep structure																				*/
void initSaleRep( int salerepid, struct salerep * ptrSalerep ) {
	ptrSalerep->salerepid = salerepid+1;
	ptrSalerep->territoryid = -1;
	ptrSalerep->amount = -1;
	ptrSalerep->filepos = -1;
}

/*------------------------------------------------------------------------------------------------------------------*/
/* initialized a new territory structure																			*/
void initTerritory( int territoryid, struct territory * ptrTerritory ) {
	ptrTerritory->territoryid = territoryid+1;
	ptrTerritory->amount = 0;
}

/*------------------------------------------------------------------------------------------------------------------*/
/* find a salerep by id																								*/
struct salerep * findSalerep( int salerepid ) {
	for ( int x = 0 ; x < structCount ; x++ ) {
		if ( salereps[x].salerepid ==  salerepid) {
			return &salereps[x];
		}
	}
	return NULL;
}

/*------------------------------------------------------------------------------------------------------------------*/
/* for a salerep, will update its data in the file															*/
void updateSaleRep(  FILE *file, struct salerep * ptrSalerep ) {

        //if the territoryid is -1, the salerep is not valid
		if ( ptrSalerep->territoryid != -1 ) {
			char line[64];
			sprintf(line, "%04d,%05d,%07ld", ptrSalerep->salerepid, ptrSalerep->territoryid, ptrSalerep->amount);
				
			fseek( file, ptrSalerep->filepos, ptrSalerep->filepos != -1 ? SEEK_SET : SEEK_END);
			fwrite(line, sizeof(char), strlen(line), file );
		}
}

/*------------------------------------------------------------------------------------------------------------------*/
/* initiliaze the salereps based on the input file
   as it reads the file, it keeps the position at which the salerep was read from 
   for updating its data once it completes processing the file
*/   
 
#define MAX_LINE_LENGHT 80

void processSalerepFile( char * filename) {

	FILE *file;
	
	file = fopen(filename, "rb");
	if (file == NULL) {
	  printf("Error opening file %s", filename);
	}	

   char line[MAX_LINE_LENGHT];


   long fileCurrentPosition = ftell(file);

   int debug = 0;
   

   while ( fgets (line, MAX_LINE_LENGHT, file) !=NULL ) {
        printf("DEBUG SALE %d\n", debug);
        debug++;
		printf("[%s] position [%ld]\n", line, fileCurrentPosition);
		
		const char separator[2] = ",";
		char *token;
		int   tokenCount=1;
		struct salerep * salerep = NULL;		

		
		token = strtok(line, separator);
   
		
		int salerepid;
		int territoryid;
		long amount;
		while( token != NULL )  {
			printf( " %s\n", token );

			switch ( tokenCount ) {
				case 1 : salerepid = atoi( token ) ;
						 salerep = findSalerep( salerepid );
						 salerep->filepos = fileCurrentPosition;
						 break;
				case 2 : territoryid = atoi( token ) ;
						 salerep->territoryid = territoryid;
						 break;
				case 3 : amount = atol( token );
						 salerep->amount = amount;
						 break;
			}
			token = strtok(NULL, separator);
			tokenCount++;
		}
		fileCurrentPosition = ftell( file );
	}
	fclose(file);
}



void processTransactionFile(char * filename){
    FILE *file;
    file = fopen(filename, "r");
    if (file == NULL){
        printf("Error opening %s", filename);
    }
    printf("DEBUG: 1 :: TransactionFile\n");        // dev
    const char separator[2] = ",";
    printf("DEBUG: 1 HELLO");
    char line[MAX_LINE_LENGHT];
    long fileCurrentPosition = ftell( file );
    int inDebug = 0;        // dev
    printf("DEBUG: 2 :: TransactionFile\n");    // dev
    while( fgets(line, MAX_LINE_LENGHT, file) != NULL){
        printf("INDEBUG: 1 :: TransactionFile\n");
        char * token;
        int tokenCount = 1;
        int trxid, salerepid, transaction_type, amount;
        struct transaction * trans;   // empty struct, will be reinitialized for each line.
        struct salerep * salerep = NULL;
        token = strtok(line, separator);   // first token
        printf("INDEBUG: 2 :: TransactionFile\n");  // dev
        while(token != NULL){
            printf("INDEBUG: 3 :: TransactionFile\n");  // dev
            switch(tokenCount){
                case 1:
                    printf("INDEBUG: 4 :: TransactionFile\n");  // dev
                    printf("%s\n", token);
                    trxid = atoi(token);
                    printf("%d\n", trxid);
                    printf("INDEBUG: 5 :: TransactionFile\n");  // dev
                    trans->trxid = trxid;
                    printf("INDEBUG: 6 :: TransactionFile\n");  // dev                  
                    break;
                case 2:
                    salerepid = atoi(token);
                    trans->salerepid = salerepid;
                    salerep = findSalerep(salerepid);
                    break;
                case 3:
                    transaction_type = atoi(token);
                    trans->type = transaction_type;
                    break;
                case 4:
                    amount = atoi(token);
                    trans->amount = amount;
                    break;
            }
            token = strtok(NULL, separator);
            tokenCount++;
        }
        printf("INDEBUG: 7 :: TransactionFile\n");  // dev
        if(trans->type == 1){
            territories->territoryid = salerep->territoryid;
            territories->amount += trans->amount;
            salerep->amount += trans->amount;
        }
        if(trans->type == 2){
            territories->territoryid = salerep->territoryid;
            territories->amount += trans->amount;
            salerep->amount += (110 / 100) * (trans->amount);
        }
        if(trans->type == 3){
            territories->territoryid = salerep->territoryid;
            territories->amount -= trans->amount;
            salerep->amount -= trans->amount;
        }
        if(trans->type == 4){
            territories->territoryid = salerep->territoryid;
            territories->amount -= trans->amount;
            salerep->amount -= (125 / 100) * trans->amount;
        }
        if(trans->type == 5){
            territories->territoryid = salerep->territoryid;
            territories->amount -= trans->amount;
        }
        if(trans->type == 6){
            territories->territoryid = salerep->territoryid;
            territories->amount -= trans->amount;
            salerep->amount -= (110 / 100) * trans->amount;
        }
        if(trans->type == 7){
            territories->territoryid = salerep->territoryid;
            salerep->amount += (75 / 100) * trans->amount;
        }
    }

}

void terr_add(FILE *file, struct territory * ter){
	fprintf(file, "%d,%ld", ter->territoryid, ter->amount);
}


int main ( int arc, char *argv[] ) {
	
	structCount = atoi(argv[1]); 

	salereps    = (struct salerep *) malloc( sizeof(struct salerep) * structCount);
	territories = (struct territory *) malloc( sizeof(struct territory) * structCount);
    printf("SIZE of salerep %lu\n", sizeof(salereps));
    printf("DEBUG: 1\n");

	for ( int x = 0 ; x < structCount ; x++ ) {
		initSaleRep( x, &salereps[x] );
		initTerritory( x, &territories[x] );
	}

    printf("DEBUG: 2\n");

	char * fileName = argv[2];

    printf("DEBUG: 3\n");

	char * filename = argv[4];

	FILE *fptr;
   	fptr = fopen(argv[4],"w");
   	if(fptr==NULL){
    	printf("Error!");
      	exit(1);
   }
   	terr_add(fptr, territories);
   	fclose(fptr);
   	printf("%s \n", argv[3]);
   	processTransactionFile(filename);
   	printf("DEBUG: 3\n");
	processSalerepFile(fileName);
	

	
	//update the salerep file with the new amount
	FILE * file = fopen(argv[2], "r+b");
	if (file == NULL) {
	  printf("Error opening file %s", fileName);
	  return -1;
	}	
	for ( int x = 0 ; x < structCount ; x++ ) {
		updateSaleRep( file, &salereps[x] );
	}
	fclose(file);
	printf("DEBUG: 4\n");
	//sort salereps
	qsort(salereps, structCount, sizeof(struct salerep), comparefunction);
    qsort(territories, structCount, sizeof(struct territory), comparefunction);

    for(int i = 0; i<structCount; i++){
        printf("%d,%ld\n", territories[i].territoryid, territories[i].amount);
    }
	

    for(int i = 0; i<structCount; i++){
        printf("%d,%ld\n", salereps[i].salerepid, salereps[i].amount);
    }
}
