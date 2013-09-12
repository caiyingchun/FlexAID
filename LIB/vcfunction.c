#include "Vcontacts.h"

#define DEBUG_LEVEL 0

int vcfunction(FA_Global* FA,VC_Global* VC,atom* atoms,resid* residue, vector< pair<int,int> > & intraclashes)
{
	int    rnum=0;
	int    type=1;
		
	// reset all values pointed
	memset(VC->Calc,0,FA->atm_cnt_real*sizeof(atomsas));
	memset(VC->Calclist,0,FA->atm_cnt_real*sizeof(int));
	memset(VC->ca_index,0,FA->atm_cnt_real*sizeof(int));
	memset(VC->seed,0,3*FA->atm_cnt_real*sizeof(int));
	memset(VC->contlist,0,10000*sizeof(contactlist));
	memset(FA->contacts,0,100000*sizeof(int));

	// reset CF values
	for(int j=0; j<FA->num_optres; ++j){
		FA->optres[j].cf.rclash=0;
		FA->optres[j].cf.wal=0.0;
		FA->optres[j].cf.com=0.0;
		FA->optres[j].cf.con=0.0;
		FA->optres[j].cf.sas=0.0;
	}
	
	double permea = (double)FA->permeability;
	double dee_clash = (double)FA->dee_clash;
	
	// allocate
	//float  matrix[FA->ntypes*FA->ntypes];
	/*
	// empty surface matrix values
	for(i=0;i<FA->ntypes;++i){
	for(j=0;j<FA->ntypes;++j){
	matrix[i][j]=0.0;
	}
	}
	*/  
    
	//printf("=============NEW INDIVIDUAL==============\n");
    
	if(Vcontacts(FA,atoms,residue,VC) == -1){

        FA->skipped++;

		free(VC->ca_rec);
		free(VC->box);
		
		return(1);
	}
	
	for(int i=0; i<FA->atm_cnt_real; ++i) {
		
		cfstr* cfs = NULL;
#if DEBUG_LEVEL > 0
		cfstr cfs_atom;
#endif

		// atom from which contacts are calculated
		int atomzero = FA->num_atm[VC->Calc[i].atomnum];
		
		// number of constraints for atomzero
		int nconszero = atoms[atomzero].ncons;
		
		if(atoms[atomzero].optres != NULL)
		{
			// the residue optimizable
			rnum = atoms[atomzero].optres->rnum;
			type = atoms[atomzero].optres->type;
			cfs = &atoms[atomzero].optres->cf;
		}
		else
		{
			continue;
		}

		if(type == 0 && atoms[atomzero].isbb)
		{
			continue;
		}
		
		//printf("-------------------------------\nAtom[%4d]=%s in Residue[%4d]\n",VC->Calc[i].atomnum,atoms[FA->num_atm[VC->Calc[i].atomnum]].name,VC->Calc[i].resnum);
		
		
#if DEBUG_LEVEL > 0
		double com_atm=0.0;
		double Ewall_atm=0.0;
#endif
		
		double radA  = (double)VC->Calc[i].radius;
		double radoA = radA + Rw;
		
		double SAS = 4.0*PI*radoA*radoA;
		double surfA = SAS;
		
#if DEBUG_LEVEL > 0
		cfs_atom.sas = 0.0;
#endif

		if(atoms[atomzero].ncons > 0){

			for(int j=0;j<atoms[atomzero].ncons;j++){
	
				/*
				double radC = atoms[atomzero].number==atoms[atomzero].cons[j]->anum1 ? 
					(double)atoms[FA->num_atm[atoms[atomzero].cons[j]->anum2]].radius:
					(double)atoms[FA->num_atm[atoms[atomzero].cons[j]->anum1]].radius;
				*/
				
				// maximum penalty value (starting penalty)
				// default value if atoms are not interacting
				//cfs->con += KANGLE*(radA+radC+2.0*Rw);

				if(atoms[atomzero].cons[j]->type == 1){
					cfs->con += KDIST;
				}
				
				//printf("constraint for atom[%d]: %.3f\n", atoms[atomzero].number,cfs->con);
			}
      
			/*
			  printf("default constraint value: %.3f\n",cfs->con);
			  getchar();
			*/
		}


#if DEBUG_LEVEL > 0
		printf("==================================================================================\n");
		printf("ATOM :: RES C RNUM  ANUM  T  RAD ::   COMPL  (W)  DIST   AREA ::     CF.COM     CF.WAL\n");
		printf("----------------------------------------------------------------------------------\n");
		printf("ATOM :: %3s %c %4d %5d %2d %4.2f\n",
		       residue[atoms[atomzero].ofres].name,
		       residue[atoms[atomzero].ofres].chn,
		       residue[atoms[atomzero].ofres].number,
		       atoms[atomzero].number,
		       atoms[atomzero].type,
		       atoms[atomzero].radius);
#endif


		int currindex = VC->ca_index[i];
		int contnum = 0;  // number of contacts (excluding bloops away atoms)

		while(currindex != -1) {
			
			double radB  = (double)VC->Calc[VC->ca_rec[currindex].atom].radius;
			double radoB = radB + Rw;
			double surfB = 4.0*PI*radoB*radoB;
			
			double rAB   = radA+radB;
			
			//double complementarity = 0.0;
			double area = VC->ca_rec[currindex].area;
			
			struct energy_matrix* energy_matrix = &FA->energy_matrix[(VC->Calc[i].type-1)*FA->ntypes +
										 (VC->Calc[VC->ca_rec[currindex].atom].type-1)];
			double yval = get_yval(energy_matrix,area/((surfA+surfB)/2.0));
			
			SAS -= area;
			
			// number of contacts counter
			contnum++;

#if DEBUG_LEVEL > 0
			cfs_atom.com  =  0.0;
			cfs_atom.wal  =  0.0;
#endif
			// atom in contact with atom zero
			int atomcont = FA->num_atm[VC->Calc[VC->ca_rec[currindex].atom].atomnum];

			int intramolecular = atoms[atomcont].ofres == atoms[atomzero].ofres;
	
			// get first atom of residue
			int fatm = residue[rnum].fatm[0];
		
			// is contact atom bonded to atom zero
			// if YES, skip contact atom
			if(intramolecular)
			{
				if(residue[rnum].bonded[atomcont-fatm][atomzero-fatm] >= 0)
				{

#if DEBUG_LEVEL > 2
					printf("    (B) %3s %c %4d %5d %2d %4.2f :: %7.4f (%s) %6.2f %6.2f :: %10.3f %10.3f\n",
					       VC->Calc[VC->ca_rec[currindex].atom].res,
					       VC->Calc[VC->ca_rec[currindex].atom].chn,
					       VC->Calc[VC->ca_rec[currindex].atom].resnum,
					       VC->Calc[VC->ca_rec[currindex].atom].atomnum,
					       VC->Calc[VC->ca_rec[currindex].atom].type,
					       VC->Calc[VC->ca_rec[currindex].atom].radius,
					       
					       yval, energy_matrix->weight ? "Y": "N",
					       VC->ca_rec[currindex].dist,
					       VC->ca_rec[currindex].area,
					       
					       cfs_atom.com, cfs_atom.wal);

#endif
					currindex = VC->ca_rec[currindex].prev;
					continue;	  
				}
			}

			if(FA->contacts[VC->Calc[VC->ca_rec[currindex].atom].atomnum]){
				//printf("%d already calculated\n",VC->Calc[VC->ca_rec[currindex].atom].atomnum );
				currindex = VC->ca_rec[currindex].prev;
				continue;
			}

			// covalently bonded flag
			bool covalent = false;

			// number of constraints for contact atom
			int nconscont = atoms[atomcont].ncons;
      			double dist_opt = 0.0;
			
			// do contacting atoms have the same constraint
			constraint* cons = NULL;
			if(nconszero > 0 && nconscont > 0){
				for(int j=0;j<nconszero;j++){
					for(int k=0;k<nconscont;k++){
						if(atoms[atomzero].cons[j]->id == atoms[atomcont].cons[k]->id){
							cons = &FA->constraints[atoms[atomzero].cons[j]->id];
							break;
						}
					}
					if(cons != NULL){break;}
				}
				
				if(cons != NULL){
					
					if(cons->type == 1){
						
						covalent = true;
						dist_opt = cons->bond_len;
						
						//ang = angle(atoms[atomzero].coor,atoms[atomcont].coor,atoms[atoms[atomcont].bond[1]].coor);
						//cfs->con -= KANGLE*cons->max_ang*GetValueFromGaussian(ang,120.0,cons->max_ang);
						
						//float gaus=GetValueFromGaussian(VC->ca_rec[currindex].dist,dist_opt,cons->max_dist);
						//printf("gaus: %.2f\n",gaus);
						//getchar();
						
						printf("cons->max_dist=%.3f - VC->ca_rec[currindex].dist=%.3f - dist_opt=%.3f - GetValueFromGaussian=%.3f\n", cons->max_dist, VC->ca_rec[currindex].dist, dist_opt,GetValueFromGaussian(VC->ca_rec[currindex].dist,dist_opt,cons->max_dist));
						
						printf("before: %.3f\n",cfs->con);
						cfs->con -= KDIST * GetValueFromGaussian(VC->ca_rec[currindex].dist,dist_opt,cons->max_dist);
						printf("after: %.3f\n",cfs->con);
						//getchar();
						
						/*
						  printf("removing from con: %.3f\n", //KANGLE*cons->max_ang*GetValueFromGaussian(ang,120.0,cons->max_ang));
						  KDIST*cons->max_dist*GetValueFromGaussian(VC->ca_rec[currindex].dist,dist_opt,cons->max_dist));
						  getchar();
						*/
						
					}else{

						; // not applied anymore
						/*
						// interaction constraint
						if(cons->force_interaction){
							complementarity = 
								complementarity < 0 ? 
								-1.0 * complementarity:
								complementarity;
						}

						complementarity *= cons->interaction_factor;
						*/
						
						/*
						  printf("found interaction constraint: %.2f (%d)\n",
						  cons->interaction_factor,cons->force_interaction);
						*/
					}

				}
	
				//printf("constraint[%d] applies.\n",cons->id);
			}
            
			//	coorB = VC->Calc[VC->ca_rec[currindex].atom].coor;

			// CHECK IF CLASH
			float clash_distance = permea*rAB;
			if(covalent && permea*dist_opt < permea*rAB){
				clash_distance = permea*dist_opt;
			}
                        
			if (VC->ca_rec[currindex].dist < clash_distance){
				
				/*
				  if(covalent){
				  int atmi=FA->num_atm[VC->Calc[i].atomnum];
				  int atmj=FA->num_atm[VC->Calc[VC->ca_rec[currindex].atom].atomnum];
				  int resi=atoms[atmi].ofres;
				  int resj=atoms[atmj].ofres;
				  
				  printf("((Atom overlap between %s%d%c[%d](%s)-%s%d%c[%d](%s))) with DIST=%.3f with CLASH_DIST=%.3f\n",
				  residue[resi].name,residue[resi].number,residue[resi].chn,
				  atoms[atmi].number,atoms[atmi].name,
				  residue[resj].name,residue[resj].number,residue[resj].chn,
				  atoms[atmj].number,atoms[atmj].name,
				  VC->ca_rec[currindex].dist,clash_distance);
				  }
				*/
				
				double Ewall = KWALL*(pow(VC->ca_rec[currindex].dist,-12.0)-pow(permea*rAB,-12.0));
				
				cfs->wal += Ewall;

#if DEBUG_LEVEL > 0
				Ewall_atm += Ewall_atm;
				cfs_atom.wal += Ewall;
#endif
				// ligand intramolecular clash exceeds threshold
				// add an entry in the dee elimination
				if(intramolecular && type == 1 && Ewall > DEE_WALL_THRESHOLD){
					intraclashes.push_back(pair<int,int>(atomzero-fatm,atomcont-fatm));
				}
				
				// Treat everything as rigid
				if ( VC->ca_rec[currindex].dist <= dee_clash*rAB ) { cfs->rclash=1; }

			}
            
			if( !covalent ) {
				if(energy_matrix->weight){
					cfs->com += yval * area;
				}else{
					cfs->com += yval;
				}
				
#if DEBUG_LEVEL > 0	
				if(energy_matrix->weight){
					cfs_atom.com += yval * area;
				}else{
					cfs_atom.com += yval;
				}
#endif
			}
      
			/*
			// generate surface area matrix
			matrix[VC->Calc[i].type][VC->Calc[VC->ca_rec[currindex].atom].type] += area;
      
			if(VC->Calc[i].type != VC->Calc[VC->ca_rec[currindex].atom].type){
			matrix[VC->Calc[VC->ca_rec[currindex].atom].type][VC->Calc[i].type] += area;
			}
			*/
			
#if DEBUG_LEVEL > 0
			printf("        %3s %c %4d %5d %2d %4.2f :: %7.4f (%s) %6.2f %6.2f :: %10.3f %10.3f\n",
			       VC->Calc[VC->ca_rec[currindex].atom].res,
			       VC->Calc[VC->ca_rec[currindex].atom].chn,
			       VC->Calc[VC->ca_rec[currindex].atom].resnum,
			       VC->Calc[VC->ca_rec[currindex].atom].atomnum,
			       VC->Calc[VC->ca_rec[currindex].atom].type,
			       VC->Calc[VC->ca_rec[currindex].atom].radius,
			       
			       yval, energy_matrix->weight ? "Y": "N",
			       VC->ca_rec[currindex].dist,
			       VC->ca_rec[currindex].area,
			       
			       cfs_atom.com, cfs_atom.wal);
			
#endif

			// skip to next contact
			currindex = VC->ca_rec[currindex].prev;
		}    
        
		//    printf("Atom[%d]=%d has %d contacts\n",VC->Calc[i].atomnum,VC->Calc[i].atomnum,contnum);
		//    printf("Atom[%d] COM=[%8.2f]\tWAL=[%8.2f]\n",VC->Calc[i].atomnum,com_atm,Ewall_atm);
    
		if(SAS < 0.0){ SAS = 0.0; }

		if(FA->solventterm)
			cfs->sas += (double)FA->solventterm * SAS;
		else {
			struct energy_matrix* energy_matrix = &FA->energy_matrix[(VC->Calc[i].type-1)*FA->ntypes +
										 (FA->ntypes-1)];
			double yval = get_yval(energy_matrix,SAS/surfA);
			
			if(energy_matrix->weight)
				cfs->sas += yval * SAS;
			else cfs->sas += yval;
		}
		
		FA->contacts[VC->Calc[i].atomnum] = 1;
		
#if DEBUG_LEVEL > 1
		printf("CF.SAS is %.3f (Area=%.3f) for %d contacts\n", 
		       cfs_atom.sas, SAS, contnum); 
#endif

	}


	// Penalize Freesurf.
  
	//printf("FREESURF(SAS)=[%8.2f]\n",SAStot);
	//printf("FINAL SUM COM=[%8.2f]\tWAL=[%8.2f]\n",com,Ewall);
  
	//print_surfmat(matrix,"surf.mat");
#if DEBUG_LEVEL > 0
	printf("\n");
	printf("CF.sum = %.3f\n", cfs->com + cfs->sas + cfs->wal);
	printf("CF.com = %.3f\n", cfs->com);
	printf("CF.sas = %.3f\n", cfs->sas);
	printf("CF.wal = %.3f\n", cfs->wal);
	getchar(); 
#endif
  	
	free(VC->ca_rec);
	//printf("free-ing %p\n",VC->ca_rec);
  
	free(VC->box);

  
	return(0);
  
}

double get_yval(struct energy_matrix* energy_matrix, double relative_area)
{
	double yval = 0.0;
	if(energy_matrix->weight)
		yval = energy_matrix->energy_values->y;
	else {
		struct energy_values* xyval = energy_matrix->energy_values;
		yval = xyval->y;
		while(relative_area > xyval->x && xyval->next_value != NULL){
			/*
			  printf("x=%.3f next_value.x=%.3f next_value.y=%.3f\n",
			  xyval->x, xyval->next_value->x, xyval->next_value->y);
			*/
			yval = xyval->y;
			xyval = xyval->next_value;
		}
		yval += xyval->y;
		yval /= 2.0;
		
		/*
		if(energy_matrix->type2 == 40){
			printf("stopped at x=%.3f with y=%.3f\n", xyval->x, xyval->y);
			printf("prob func. yval=%.3f for relative_area %.3f for [%d][%d]\n", yval, relative_area,
			       energy_matrix->type1, energy_matrix->type2);
			printf("calculated y=%.3f\n", yval);
		}
		*/
	}

	return yval;
}
