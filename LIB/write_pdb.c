#include "flexaid.h"
#include "boinc.h"

/******************************************************************************
 * SUBROUTINE write_pdb writes a PDB file
 ******************************************************************************/
int write_pdb(FA_Global* FA,atom *atoms, resid* residue,char outfile[], char remark[]){
	char field[7];
	int rot;
	
	//printf("will write pdb!\n");
	FILE *outfile_ptr = NULL;
	if(!OpenFile_B(outfile,"w",&outfile_ptr)){
		Terminate(6);
	}else{				
		// write header information
		fprintf(outfile_ptr,"REMARK coordinate file generated by FlexAID\n");
		fprintf(outfile_ptr,"%s\n",remark);
		
                //printf("outfile=%s\n", outfile);
		if(FA->contributions){
			write_contributions(FA,outfile_ptr,true);
			write_contributions(FA,outfile_ptr,false);
		}
		
		for(int k=1;k<=FA->res_cnt;k++){
			rot=residue[k].rot;
			
			/*
			printf("write_pdb: rot is %d - fatm: %d - latm: %d - atoms[%d].number = %d\n",
			       rot,residue[k].fatm[rot],residue[k].latm[rot],
			       residue[k].fatm[rot],atoms[residue[k].fatm[rot]].number);
			*/
			
			for(int i=residue[k].fatm[rot];i<=residue[k].latm[rot];i++){
                
				if(!FA->output_scored_only || atoms[i].optres != NULL){
                    
					if(residue[atoms[i].ofres].type==0){strcpy(field,"ATOM  ");}
					if(residue[atoms[i].ofres].type==1){strcpy(field,"HETATM");}
					
					fprintf(outfile_ptr,"%s%5d %s %s %c%4d    %8.3f%8.3f%8.3f  1.00  0.00\n",
						field,
						atoms[i].number,
						atoms[i].name,
						residue[atoms[i].ofres].name,
						residue[atoms[i].ofres].chn,
						residue[atoms[i].ofres].number,
						atoms[i].coor[0],
						atoms[i].coor[1],
						atoms[i].coor[2]
						);
					
				}
			}
		}
		
		//view the protein center geometry
		//fprintf(outfile_ptr, "HETATM99999  C   ORI -9999    %8.3f%8.3f%8.3f  1.00  0.00\n",
		//	    FA->ori[0],FA->ori[1],FA->ori[2]);
        
		// write conect data for all ligands
		for(int i=1;i<=FA->num_het;i++){
			rot=residue[FA->het_res[i]].rot;
            
			for(int j=residue[FA->het_res[i]].fatm[rot];j<=residue[FA->het_res[i]].latm[rot];j++){
                
				fprintf(outfile_ptr,"CONECT%5d",atoms[j].number);
				for(int k=1;k<=atoms[j].bond[0];k++){
					fprintf(outfile_ptr,"%5d",atoms[atoms[j].bond[k]].number);
				}
				fprintf(outfile_ptr,"\n");
			}
		}
	}
	
	fprintf(outfile_ptr, "END\n");
	
	CloseFile_B(&outfile_ptr,"w");
    
	return(0);
}

void write_contributions(FA_Global* FA,FILE* outfile_ptr, bool positive){

	float* contributions[MAX_CONTRIBUTIONS];	
	int ncont = 0;
	
	fprintf(outfile_ptr,"REMARK Most important %s interaction contributions to binding\n",
		positive ? "POSITIVE (favourable)": "NEGATIVE (unfavourable)");
	
	while(ncont < MAX_CONTRIBUTIONS){
		float* contribution=NULL;
		int k=0,l=0;
		for(int i=0; i<FA->ntypes; i++){
			for(int j=i; j<FA->ntypes; j++){
				if(contribution == NULL || (
					   (positive && FA->contributions[i*FA->ntypes+j] < *contribution) ||
					   (!positive && FA->contributions[i*FA->ntypes+j] > *contribution)
					   )){
					
					bool exists = false;
					int m = 0;
					while(m < ncont){
						if(contributions[m] == &FA->contributions[i*FA->ntypes+j]){
							exists = true;
							break;
						}
						m++;
					}
					
					if(!exists){
						contribution = &FA->contributions[i*FA->ntypes+j];
						k=i+1; l=j+1;
					}
				}
			}
		}
		
		if(*contribution != 0.0){
			contributions[ncont] = contribution;
			fprintf(outfile_ptr, "%3d: %2d-%2d with energy of %.3f\n", ++ncont, k,l, *contribution);
		}else{
			break;
		}
	}
}
