#ifndef BINDINGMODE_H
#define BINDINGMODE_H

#include "gaboom.h"
#include "boinc.h"

//#define UNDEFINED_DIST FLT_MAX // Defined in FOPTICS as > than +INF
#define UNDEFINED_DIST -0.1f // Defined in FOPTICS as > than +INF
#define isUndefinedDist(a) ((a - UNDEFINED_DIST) <= FLT_EPSILON)

class BindingPopulation; // forward-declaration in order to access BindingPopulation* Population pointer
/*****************************************\
				  Pose
\*****************************************/
struct Pose
{
	// friend class BindingPopulation;
	
	// public constructor :
	Pose(chromosome* chrom, int chrom_index, int order, float dist, uint temperature, std::vector<float>);
	~Pose();
	// public (default behavior when struct is used instead of class)
	int chrom_index;
	int order;
	float reachDist;
	chromosome* chrom;
	double CF;
	double boltzmann_weight;
	std::vector<float> vPose;
	inline bool const operator< (const Pose& rhs);
	inline bool const operator> (const Pose& rhs);
	inline bool const operator==(const Pose& rhs);
};
inline bool const operator==(const Pose& lhs, const Pose& rhs);
struct PoseClassifier
{
   inline bool operator() ( const Pose& Pose1, const Pose& Pose2 )
   {
       	if(Pose1.order < Pose2.order) return true;
       	else if(Pose1.order > Pose2.order) return false;
		
		if(Pose1.reachDist < Pose2.reachDist) return true;
		else if(Pose1.reachDist > Pose2.reachDist) return false;
		
		if(Pose1.chrom_index < Pose2.chrom_index) return true;
		else if(Pose1.chrom_index > Pose2.chrom_index) return false;
		
		return false;
   }
};
/*****************************************\
			  BindingMode
\*****************************************/
class BindingMode // aggregation of poses (Cluster)
{
	friend class BindingPopulation;
	friend class FastOPTICS;
	
	public:
												// public constructor (explicitely needs a pointer to a BindingPopulation of type BindingPopulation*)
		explicit 								BindingMode(BindingPopulation*);

			void								add_Pose(Pose&);
												//	tells if a Pose can be added to a BidingMode
			bool 								isPoseAggregable(const Pose& pose) const;
												//	tells if the BindingMode contains one *homogenic* population
			bool 								isHomogenic() const;
			void								clear_Poses();
			int									get_BindingMode_size() const;
			float 								compute_distance(const Pose& pose1, const Pose& pose2) const;
			double								compute_energy() const;
			double								compute_entropy() const;
			double								compute_enthalpy() const;
			std::vector<Pose>::const_iterator 	elect_Representative(bool useOPTICSordering) const;
			inline bool const 					operator<(const BindingMode&);


 	protected:
		std::vector<Pose> Poses;
		BindingPopulation* Population; // used to access the BindingPopulation

		void	set_energy();

	private:
		void 	output_BindingMode(int num_result, char* end_strfile, char* tmp_end_strfile, char* dockinp, char* gainp, int minPoints);
		void	output_dynamic_BindingMode(int nBindingMode, char* end_strfile, char* tmp_end_strfile, char* dockinp, char* gainp, int minPoints);
		double energy;
};

/*****************************************\
			BindingPopulation  
\*****************************************/
class BindingPopulation
{
	friend class BindingMode;
    friend class FastOPTICS;
	public:
		// Temperature is used for energy calculations of BindingModes
		unsigned int Temperature;
		
		explicit 	BindingPopulation(FA_Global* FA, GB_Global* GB, VC_Global* VC, chromosome* chrom, genlim* gene_lim, atom* atoms, resid* residue, gridpoint* cleftgrid, int nChrom);
			// 	add new binding mode to population
		void	add_BindingMode(BindingMode&);
			// 	classify the Pose into the appropriate BindingMode in the BindingPopulation
		void 	Classify_Pose(Pose& pose);
			// 	classify the whole BindingPopulation into BindingModes (this method might call Classify_Pose())
		void 	Classify_Population();
			// 	return the number of BindinMonde (size getter)
		int		get_Population_size();
			// 	output BindingMode up to nResults results
		void	output_Population(int nResults, char* end_strfile, char* tmp_end_strfile, char* dockinp, char* gainp, int minPoints);

	protected:
		double PartitionFunction;	// sum of all Boltzmann_weight
		int nChroms;				// n_chrom_snapshot input to clustergin function

		// FlexAID pointer
		FA_Global* 	FA;			// pointer to FA_Global struct
		GB_Global* 	GB;			// pointer to GB_Global struct
		VC_Global* 	VC;			// pointer to VC_Global struct
		chromosome* chroms;		// pointer to chromosomes' array
		genlim* gene_lim;		// pointer to gene_lim genlim array (useful for bondaries defined for each gene)
		atom* atoms;			// pointer to atoms' array
		resid* residue;			// pointer to residues' array
		gridpoint* cleftgrid;	// pointer to gridpoints' array (defining the total search space of the simulation)
        void 						Entropize(); 	// Sort BindinModes according to their observation frequency
	private:
		std::vector< BindingMode > 	BindingModes;	// BindingMode container
		
		
		struct EnergyComparator
		{
			inline bool operator() ( const BindingMode& BindingMode1, const BindingMode& BindingMode2 )
			{
				return (BindingMode1.compute_energy() < BindingMode2.compute_energy());
			}
		};
};
#endif
