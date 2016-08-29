#include "BindingMode.h"

/*****************************************\
			BindingPopulation  
\*****************************************/
// public (explicitely requires unsigned int temperature) *non-overloadable*
// BindingPopulation::BindingPopulation(unsigned int temp) : Temperature(temp)
BindingPopulation::BindingPopulation(FA_Global* pFA, GB_Global* pGB, VC_Global* pVC, chromosome* pchrom, genlim* pgene_lim, atom* patoms, resid* presidue, gridpoint* pcleftgrid, int num_chrom) : Temperature(pFA->temperature), PartitionFunction(0.0), nChroms(num_chrom), FA(pFA), GB(pGB), VC(pVC), chroms(pchrom), gene_lim(pgene_lim), atoms(patoms), residue(presidue), cleftgrid(pcleftgrid)
{
}


void BindingPopulation::add_BindingMode(BindingMode& mode)
{
	for(std::vector<Pose>::iterator pose = mode.Poses.begin(); pose != mode.Poses.end(); ++pose)
	{
		this->PartitionFunction += pose->boltzmann_weight;
	}
    mode.set_energy();
	this->BindingModes.push_back(mode);
//	this->Entropize();
}

bool BindingPopulation::merge_BindingModes(std::vector< BindingMode >::iterator mode1, std::vector< BindingMode >::iterator mode2)
{
	// assign BindingMode* pointers to mode1 and mode 2
	// BindingMode::BindingMode Current(this);
	BindingMode Current = BindingMode(this);
	// BindingMode Current = BindingMode::BindingMode(this);

	// if necessary, exchange pointers in order to merge Poses into the lowest energy BindingMpde
    for(std::vector< Pose >::iterator itp = mode1->Poses.begin(); itp != mode1->Poses.end(); ++itp)
    {
    	Current.add_Pose(*itp);
    }
    for(std::vector< Pose >::iterator itp = mode2->Poses.begin(); itp != mode2->Poses.end(); ++itp)
    {
    	Current.add_Pose(*itp);
    }
    // check if the BindingMode is homogenic enough to be added and to consider 
    // the merger of *mode2 into *mode1 as successful
    if(Current.isHomogenic())
    {
    	for(std::vector<Pose>::iterator itp = mode2->Poses.begin(); itp != mode2->Poses.end(); ++itp)
    	{
    		mode1->add_Pose(*itp);
    	}
    	// invalidate BindingMode pointed by mode2 (all its poses are added into *mode1)
    	mode2->isValid = false;
    	// return that the merge was successful (as of now, the return value is unused tho)
    	return true;
    }
    else
    {
    	//
    	return false;
    }
}

void BindingPopulation::remove_BindingMode(std::vector<BindingMode>::iterator mode)
{
	this->BindingModes.erase(mode);
}

void BindingPopulation::remove_invalid_BindingModes()
{
	for(std::vector< BindingMode >::iterator it = this->BindingModes.begin(); it != this->BindingModes.end(); ++it)
	{
		// the call to remove_BindingMode() might seem superfluous as of now,
		// but could later be useful to call as a standalone function so I left it.
		if(!it->isValid) this->remove_BindingMode(it);
	}
}

void BindingPopulation::Classify_BindingModes()
{
	float sizeTolerance = (3-static_cast<float>(2.0f/3.0f)) * this->FA->cluster_rmsd;
	for(std::vector<BindingMode>::iterator it1 = this->BindingModes.begin(); it1 != this->BindingModes.end(); ++it1)
	{
		for(std::vector<BindingMode>::iterator it2 = this->BindingModes.begin(); it2 != this->BindingModes.end(); ++it2)
		{
			// do not proceed further if (*it1) and (*it2) are the same BindingMode
			if((*it1) == (*it2)) continue;

			// do not proceed further if any of the two BindingModes isn't valid
			else if(!it1->isValid || !it2->isValid) continue;

			else if(this->compute_distance((*it1->elect_Representative(false)), (*it2->elect_Representative(false))) <= sizeTolerance) 
			{
				// need to check the merging process because the merging process could invalidate iterators
				this->merge_BindingModes(it1, it2);
			}


		}
	}
	// upon merging the BindingModes A and B, all the poses found in B are added to A
	// thus invaliding the BindingMode B (all of its poses are now part of A and B should not be considered)
	this->remove_invalid_BindingModes();
}

void BindingPopulation::Entropize()
{
	for(std::vector<BindingMode>::iterator it = this->BindingModes.begin(); it != this->BindingModes.end(); ++it)
	{
		it->set_energy();
	}
	// sort BindingModes using customly defined BindingPopulation::EnergyComparator's comparator=
	std::sort(this->BindingModes.begin(), this->BindingModes.end(), BindingPopulation::EnergyComparator());
}

float BindingPopulation::compute_distance(const Pose& pose1, const Pose& pose2) const
{
	float distance = 0.0f;
	// perform distance^2 calculation
	for(int i = 0; i < pose1.vPose.size(); ++i)
	{
		float temp = pose1.vPose[i] - pose2.vPose[i];
		distance += temp * temp;
	}
	// return square-root of distance^2
	return sqrtf(distance);
}

int BindingPopulation::get_Population_size() { return static_cast<int>(this->BindingModes.size()); }


// output BindingMode up to nResults results
void BindingPopulation::output_Population(int nResults, char* end_strfile, char* tmp_end_strfile, char* dockinp, char* gainp, int minPoints)
{
    // Output Population information ~= output clusters informations (*.cad)
    
    
    // Looping through BindingModes
    int num_result = 0;
    if(!nResults) nResults = this->get_Population_size() - 1; // if 0 is sent to this function, output all available results
	for(std::vector<BindingMode>::iterator mode = this->BindingModes.begin(); mode != this->BindingModes.end() && nResults > 0; ++mode, --nResults, ++num_result)
	{
		mode->output_BindingMode(num_result, end_strfile, tmp_end_strfile, dockinp, gainp, minPoints);
        mode->output_dynamic_BindingMode(num_result,end_strfile, tmp_end_strfile, dockinp, gainp, minPoints);
	}
}



/*****************************************\
			  BindingMode
\*****************************************/

// public constructor *non-overloadable*
BindingMode::BindingMode(BindingPopulation* pop) : Population(pop), isValid(true), energy(0.0) {}


// public method for pose addition
void BindingMode::add_Pose(Pose& pose)
{
	this->Poses.push_back(pose);
}

bool BindingMode::isPoseAggregable(const Pose& pose) const
{
	bool accept = true;
	float sizeTolerance = static_cast<float>(2.0f/3.0f);

	// automatically accep if the BindingMode is empty
	if(!this->get_BindingMode_size()) return accept;
	
	for(std::vector<Pose>::const_iterator it = this->Poses.begin(); it != this->Poses.end(); ++it)
	{
		if( this->compute_distance((*it),pose) > ((1+sizeTolerance) * this->Population->FA->cluster_rmsd) )
		{
			accept = false;
			break;
		}
	}
	
	return accept;
}

bool BindingMode::isHomogenic() const
{
	bool accept = true;
	float sizeTolerance = static_cast<float>(2.0f/3.0f);
	
	for(std::vector<Pose>::const_iterator it1 = this->Poses.begin(); it1 != this->Poses.end(); ++it1)
	{
		for(std::vector<Pose>::const_iterator it2 = this->Poses.begin(); it2 != this->Poses.end(); ++it2)
		{
			if ((*it1) == (*it2)) continue;
			else if( this->compute_distance((*it1),(*it2)) > ((3-sizeTolerance) * this->Population->FA->cluster_rmsd) )
			{	
				accept = false;
				break;
			}
		}
		if(!accept) break;
	}

	return accept;
}

float BindingMode::compute_distance(const Pose& pose1, const Pose& pose2) const
{
	float distance = 0.0f;
	// perform distance^2 calculation
	for(int i = 0; i < pose1.vPose.size() && i < pose2.vPose.size(); ++i)
	{
		float temp = pose1.vPose[i] - pose2.vPose[i];
		distance += temp * temp;
	}
	// return square-root of distance^2
	return sqrtf(distance);
}

double BindingMode::compute_enthalpy() const
{
	double enthalpy = 0.0;
	// compute enthalpy
	for(std::vector<Pose>::const_iterator pose = this->Poses.begin(); pose != this->Poses.end(); ++pose)
	{
		double boltzmann_prob = pose->boltzmann_weight / this->Population->PartitionFunction;
		enthalpy += boltzmann_prob * pose->CF;
	}
	return enthalpy;
}


double BindingMode::compute_entropy() const
{ 
	double entropy = 0.0;
	// compute entropy
	for(std::vector<Pose>::const_iterator pose = this->Poses.begin(); pose != this->Poses.end(); ++pose)
	{
		double boltzmann_prob = pose->boltzmann_weight / this->Population->PartitionFunction;
		entropy += boltzmann_prob * log(boltzmann_prob);
	}
	return -entropy; // returning a S value instead of ∆S value. Rendering it negative as in Gibbs Entropy (no reference state)
}


double BindingMode::compute_energy() const
{ 
	double energy = ( this->compute_enthalpy() - ( this->Population->Temperature * this->compute_entropy() ) );
	return energy;
}


int BindingMode::get_BindingMode_size() const { return static_cast<int>(this->Poses.size()); }


void BindingMode::clear_Poses() { this->Poses.clear(); }


void BindingMode::set_energy()
{
	this->energy = this->compute_energy();
}


void BindingMode::output_BindingMode(int num_result, char* end_strfile, char* tmp_end_strfile, char* dockinp, char* gainp, int minPoints)
{
    // File and Output variables declarations
    cfstr CF; /* complementarity function value */
    resid *pRes = NULL;
    cfstr* pCF = NULL;

    char sufix[25];
    char remark[MAX_REMARK];
    char tmpremark[MAX_REMARK];
	
    // 0. elect a Pose representative (Rep) of the current BindingMode
	std::vector<Pose>::const_iterator Rep_lowCF = this->elect_Representative(false);
	std::vector<Pose>::const_iterator Rep_lowOPTICS = this->elect_Representative(true);
	
    // 1. build FA->opt_par[GB->num_genes]
	 for(int k = 0; k < this->Population->GB->num_genes; ++k) this->Population->FA->opt_par[k] = Rep_lowCF->chrom->genes[k].to_ic;
	 // for(int k = 0; k < this->Population->GB->num_genes; ++k) this->Population->FA->opt_par[k] = Rep_lowOPTICS->chrom->genes[k].to_ic;

	// 2. get CF with ic2cf() 
	CF = ic2cf(this->Population->FA, this->Population->VC, this->Population->atoms, this->Population->residue, this->Population->cleftgrid, this->Population->GB->num_genes, this->Population->FA->opt_par);
	
    // 3. print REMARKS for FA->optres (res_ptr && cf_ptr for each optimizable residue)
	strcpy(remark,"REMARK optimized structure\n");
	
	sprintf(tmpremark,"REMARK Fast OPTICS clustering algorithm used to output the lowest CF as Binding Mode representative\n");
	 // sprintf(tmpremark,"REMARK Fast OPTICS clustering algorithm used to output the lowest OPTICS ordering as Binding Mode representative\n");
	strcat(remark,tmpremark);
	
	sprintf(tmpremark,"REMARK CF=%8.5f\n",get_cf_evalue(&CF));
	strcat(remark,tmpremark);
	sprintf(tmpremark,"REMARK CF.app=%8.5f\n",get_apparent_cf_evalue(&CF));
	strcat(remark,tmpremark);
    
	for(int j = 0; j < this->Population->FA->num_optres; ++j)
	{
		pRes = &this->Population->residue[this->Population->FA->optres[j].rnum];
		pCF  = &this->Population->FA->optres[j].cf;
        
        sprintf(tmpremark,"REMARK optimizable residue %s %c %d\n", pRes->name, pRes->chn, pRes->number);
        strcat(remark,tmpremark);
        
        sprintf(tmpremark ,"REMARK CF.com=%8.5f\n", pCF->com);
        strcat(remark, tmpremark);
        sprintf(tmpremark ,"REMARK CF.sas=%8.5f\n", pCF->sas);
        strcat(remark, tmpremark);
        sprintf(tmpremark ,"REMARK CF.wal=%8.5f\n", pCF->wal);
        strcat(remark, tmpremark);
        sprintf(tmpremark ,"REMARK CF.con=%8.5f\n", pCF->con);
        strcat(remark, tmpremark);
        sprintf(tmpremark, "REMARK Residue has an overall SAS of %.3f\n", pCF->totsas);
        strcat(remark, tmpremark);
	}
    
    sprintf(tmpremark,"REMARK Binding Mode:%d Best CF in Binding Mode:%8.5f OPTICS Center (CF):%8.5f Binding Mode Total CF:%8.5f Binding Mode Frequency:%d\n",
            num_result, Rep_lowCF->CF, Rep_lowOPTICS->CF, this->compute_energy(), this->get_BindingMode_size());
    strcat(remark,tmpremark);
    for(int j=0; j < this->Population->FA->npar; ++j)
	{
		sprintf(tmpremark, "REMARK [%8.3f]\n",this->Population->FA->opt_par[j]);
		strcat(remark,tmpremark);
	}

	// 4. if(REF) prints RMSD to REF
	if(this->Population->FA->refstructure == 1)
	{
		bool Hungarian = false;
		sprintf(tmpremark,"REMARK %8.5f RMSD to ref. structure (no symmetry correction)\n",
		calc_rmsd(this->Population->FA,this->Population->atoms,this->Population->residue,this->Population->cleftgrid,this->Population->FA->npar,this->Population->FA->opt_par, Hungarian));
		strcat(remark,tmpremark);
		
		Hungarian = true;
		sprintf(tmpremark,"REMARK %8.5f RMSD to ref. structure     (symmetry corrected)\n",
		calc_rmsd(this->Population->FA,this->Population->atoms,this->Population->residue,this->Population->cleftgrid,this->Population->FA->npar,this->Population->FA->opt_par, Hungarian));
		strcat(remark,tmpremark);
	}
	sprintf(tmpremark,"REMARK inputs: %s & %s\n",dockinp,gainp);
	strcat(remark,tmpremark);
	sprintf(sufix,"_%d_%d.pdb", minPoints, num_result);
	strcpy(tmp_end_strfile,end_strfile);
	strcat(tmp_end_strfile,sufix);
	// 5. write_pdb(FA,atoms,residue,tmp_end_strfile,remark)
	write_pdb(this->Population->FA,this->Population->atoms,this->Population->residue,tmp_end_strfile,remark);
}


void BindingMode::output_dynamic_BindingMode(int num_result, char* end_strfile, char* tmp_end_strfile, char* dockinp, char* gainp, int minPoints)
{
    // File and Output variables declarations
    cfstr CF; /* complementarity function value */
    resid* pRes = NULL;
    cfstr* pCF  = NULL;

    char sufix[25];
    char remark[MAX_REMARK];
    char tmpremark[MAX_REMARK];
    int nModel = 1;
    for(std::vector<Pose>::iterator Pose = this->Poses.begin(); Pose != this->Poses.end(); ++Pose, ++nModel)
    {
    	// 1. build FA->opt_par[GB->num_genes]
		for(int k = 0; k < this->Population->GB->num_genes; ++k) this->Population->FA->opt_par[k] = Pose->chrom->genes[k].to_ic;

		// 2. get CF with ic2cf() 
		CF = ic2cf(this->Population->FA, this->Population->VC, this->Population->atoms, this->Population->residue, this->Population->cleftgrid, this->Population->GB->num_genes, this->Population->FA->opt_par);
		
	    // 3. print REMARKS for FA->optres (res_ptr && cf_ptr for each optimizable residue)
		strcpy(remark,"REMARK optimized structure\n");
		
	//	 sprintf(tmpremark,"REMARK Fast OPTICS clustering algorithm used to output the lowest CF as Binding Mode representative\n");
		 sprintf(tmpremark,"REMARK Fast OPTICS clustering algorithm used to output the lowest OPTICS ordering as Binding Mode representative\n");
		strcat(remark,tmpremark);
		
		sprintf(tmpremark,"REMARK CF=%8.5f\n",get_cf_evalue(&CF));
		strcat(remark,tmpremark);
		sprintf(tmpremark,"REMARK CF.app=%8.5f\n",get_apparent_cf_evalue(&CF));
		strcat(remark,tmpremark);
	    
		for(int j = 0; j < this->Population->FA->num_optres; ++j)
		{
			pRes = &this->Population->residue[this->Population->FA->optres[j].rnum];
			pCF  = &this->Population->FA->optres[j].cf;
	        
	        sprintf(tmpremark,"REMARK optimizable residue %s %c %d\n", pRes->name, pRes->chn, pRes->number);
	        strcat(remark,tmpremark);
	        
	        sprintf(tmpremark ,"REMARK CF.com=%8.5f\n", pCF->com);
	        strcat(remark, tmpremark);
	        sprintf(tmpremark ,"REMARK CF.sas=%8.5f\n", pCF->sas);
	        strcat(remark, tmpremark);
	        sprintf(tmpremark ,"REMARK CF.wal=%8.5f\n", pCF->wal);
	        strcat(remark, tmpremark);
	        sprintf(tmpremark ,"REMARK CF.con=%8.5f\n", pCF->con);
	        strcat(remark, tmpremark);
	        sprintf(tmpremark, "REMARK Residue has an overall SAS of %.3f\n", pCF->totsas);
	        strcat(remark, tmpremark);
		}
	    
	    for(int j=0; j < this->Population->FA->npar; ++j)
		{
			sprintf(tmpremark, "REMARK [%8.3f]\n",this->Population->FA->opt_par[j]);
			strcat(remark,tmpremark);
		}

		// 4. if(REF) prints RMSD to REF
		if(this->Population->FA->refstructure == 1)
		{
			bool Hungarian = false;
			sprintf(tmpremark,"REMARK %8.5f RMSD to ref. structure (no symmetry correction)\n",
			calc_rmsd(this->Population->FA,this->Population->atoms,this->Population->residue,this->Population->cleftgrid,this->Population->FA->npar,this->Population->FA->opt_par, Hungarian));
			strcat(remark,tmpremark);
			Hungarian = true;
			sprintf(tmpremark,"REMARK %8.5f RMSD to ref. structure     (symmetry corrected)\n",
			calc_rmsd(this->Population->FA,this->Population->atoms,this->Population->residue,this->Population->cleftgrid,this->Population->FA->npar,this->Population->FA->opt_par, Hungarian));
			strcat(remark,tmpremark);
		}
		sprintf(tmpremark,"REMARK inputs: %s & %s\n",dockinp,gainp);
		strcat(remark,tmpremark);
        
		sprintf(sufix,"_%d_MODEL_%d.pdb", minPoints, num_result);
		strcpy(tmp_end_strfile,end_strfile);
		strcat(tmp_end_strfile,sufix);
		// 5. write_pdb(FA,atoms,residue,tmp_end_strfile,remark)
		if(Pose == this->Poses.begin() && Pose+1 == this->Poses.end())
		{
			write_MODEL_pdb(true, true, nModel, this->Population->FA,this->Population->atoms,this->Population->residue,tmp_end_strfile,remark);
		}
		else if(Pose == this->Poses.begin())
		{
			write_MODEL_pdb(true, false, nModel, this->Population->FA,this->Population->atoms,this->Population->residue,tmp_end_strfile,remark);
		}
		else if(Pose+1 == this->Poses.end())
		{
			write_MODEL_pdb(false, true, nModel, this->Population->FA,this->Population->atoms,this->Population->residue,tmp_end_strfile,remark);
		}
		else
		{
			write_MODEL_pdb(false, false, nModel, this->Population->FA,this->Population->atoms,this->Population->residue,tmp_end_strfile,remark);
		}
    }
}

std::vector<Pose>::const_iterator BindingMode::elect_Representative(bool useOPTICSorder) const
{
	std::vector<Pose>::const_iterator Rep = this->Poses.begin();
	for(std::vector<Pose>::const_iterator it = this->Poses.begin(); it != this->Poses.end(); ++it)
	{
		if(!useOPTICSorder && (Rep->CF - it->CF) > DBL_EPSILON ) Rep = it;
		if(useOPTICSorder && it->reachDist < Rep->reachDist && !isUndefinedDist(it->reachDist)) Rep = it;
	}
	return Rep;
}


inline bool const BindingMode::operator< (const BindingMode& rhs) { return ( this->compute_energy() < rhs.compute_energy() ); }

inline bool const BindingMode::operator==(const BindingMode& rhs)
{
	if( this->get_BindingMode_size() == rhs.get_BindingMode_size() && this->elect_Representative(false) == rhs.elect_Representative(false) )
	{
		return true;
	}
	else return false;
}

inline bool const operator==(const BindingMode& lhs, const BindingMode& rhs)
{
	if(lhs.get_BindingMode_size() == rhs.get_BindingMode_size() && lhs.elect_Representative(false) == rhs.elect_Representative(false) )
	{
		return true;
	}
	else return false;
}

/*****************************************\
				  Pose
\*****************************************/
// public constructor for Pose *non-overloadable*
Pose::Pose(chromosome* chrom, int index, int iorder, float dist, uint temperature, std::vector<float> vec) :  chrom_index(index), order(iorder), reachDist(dist), chrom(chrom), CF(chrom->app_evalue), vPose(vec)
{
	this->boltzmann_weight = pow( E, ((-1.0) * (1/static_cast<double>(temperature)) * chrom->app_evalue) );
}

Pose::~Pose(){}

inline bool const Pose::operator< (const Pose& rhs)
{
	if(this->order < rhs.order) return true;
   	else if(this->order > rhs.order) return false;
	
	if(this->reachDist < rhs.reachDist) return true;
	else if(this->reachDist > rhs.reachDist) return false;
	
	if(this->chrom_index < rhs.chrom_index) return true;
	else if(this->chrom_index > rhs.chrom_index) return false;
	
	else return false;
}

inline bool const Pose::operator> (const Pose& rhs)
{
	if(this->order < rhs.order) return false;
   	else if(this->order > rhs.order) return true;
	
	if(this->reachDist < rhs.reachDist) return false;
	else if(this->reachDist > rhs.reachDist) return true;
	
	if(this->chrom_index < rhs.chrom_index) return false;
	else if(this->chrom_index > rhs.chrom_index) return true;
	
	else return false;
}

inline bool const Pose::operator==(const Pose& rhs)
{
	if(this->chrom_index == rhs.chrom_index) return true;
	else return false;
}

inline bool const operator==(const Pose& lhs, const Pose& rhs)
{
	if(lhs.chrom_index == rhs.chrom_index) return true;
	else return false;
}