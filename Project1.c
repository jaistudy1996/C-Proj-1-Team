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
   	/*
void processTransactionFile(char * filename){
    FILE  *file;
    file = fopen(filename, "r");
    if (file == NULL){
        printf("Error opening %s", filename);
    }
    const char separator[2] = ",";
    char line[MAX_LINE_LENGHT];
    long fileCurrentPosition = ftell( file );

    while( fgets(line, MAX_LINE_LENGHT, file) != NULL){

        char * token;
        int tokenCount = 1;
        int trxid, salerepid, transaction_type, amount;
        struct transaction * trans = NULL;   // empty struct, will be reinitialized for each line.
        struct salerep * salerep = NULL;
        token = strtok(line, separator);   // first token

        while(token != NULL){
            switch(tokenCount){
                case 1:
                    trxid = atoi(token);
                    trans->trxid = trxid;
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

        // if(trans.type == 1){

        // }
    }

}
*/
void processSalerepFile( char * filename) {

	
	FILE *file;
	
	file = fopen(filename, "rb");
	if (file == NULL) {
	  printf("Error opening file %s", filename);
	}	

   char line[MAX_LINE_LENGHT];

   long fileCurrentPosition = ftell(file);

   while ( fgets (line, MAX_LINE_LENGHT, file) !=NULL ) {
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


int main ( int arc, char *argv[] ) {
	printf("step0 \n");
	structCount = atoi(argv[1]); 

	salereps    = (struct salerep *) malloc( sizeof(struct salerep) * structCount);
	territories = (struct territory *) malloc( sizeof(struct territory) * structCount);
    
	for ( int x = 0 ; x < structCount ; x++ ) {
		initSaleRep( x, &salereps[x] );
		initTerritory( x, &territories[x] );
	}

	printf("step1\n");
	char * fileName = argv[2];
	char * filename = argv[3];
	printf("step2\n");
	
	printf("step3\n");
	processSalerepFile(fileName);
	printf("step4\n");
	
	printf("step5\n");
	

	
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
	
	//sort salereps
	qsort(salereps, structCount, sizeof(struct salerep), comparefunction);
	
	
}
