// Copyright (c) 2014-2019 Michael C. Heiber
// This source file is part of the Ising_OPV project, which is subject to the MIT License.
// For more information, see the LICENSE file that accompanies this software.
// The Ising_OPV project can be found on Github at https://github.com/MikeHeiber/Ising_OPV

#include "Morphology.h"

using namespace std;
using namespace tinyxml2;

namespace Ising_OPV {

	Morphology::Morphology() {

	}

	Morphology::Morphology(const Parameters& params, const int id) {
		if (!params.checkParameters()) {
			cout << ID << ": Error! Input parameters are invalid." << endl;
			throw invalid_argument("Error! Input parameters are invalid.");
		}
		ID = id;
		Params = params;
		Lattice::Lattice_Params lattice_params;
		lattice_params.Enable_periodic_x = true;
		lattice_params.Enable_periodic_y = true;
		lattice_params.Enable_periodic_z = params.Enable_periodic_z;
		lattice_params.Length = params.Length;
		lattice_params.Width = params.Width;
		lattice_params.Height = params.Height;
		lattice_params.Unit_size = 1.0;
		lattice.init(lattice_params);
		gen.seed((int)time(0)*(id + 1));
	}

	Morphology::Morphology(const Lattice& input_lattice, const Parameters& params, const int id) {
		if (!params.checkParameters()) {
			cout << ID << ": Error! Input parameters are invalid." << endl;
			throw invalid_argument("Error! Input parameters are invalid.");
		}
		if (input_lattice.getLength() != params.Length || input_lattice.getWidth() != params.Width || input_lattice.getHeight() != params.Height || input_lattice.isZPeriodic() != params.Enable_periodic_z) {
			cout << ID << ": Error! Input parameters do not agree with the dimensions or periodic boundary conditions of the input Lattice object." << endl;
			throw invalid_argument("Error! Input parameters do not agree with the dimensions or periodic boundary conditions of the input Lattice object.");
		}
		ID = id;
		Params = params;
		lattice = input_lattice;
		gen.seed((int)time(0)*(id + 1));
		for (int i = 0; i < (int)lattice.getNumSites(); i++) {
			bool type_found = false;
			for (int n = 0; n < (int)Site_types.size(); n++) {
				if (lattice.getSiteType(i) == Site_types[n]) {
					type_found = true;
					break;
				}
			}
			if (!type_found) {
				addSiteType(lattice.getSiteType(i));
			}
		}
	}

	Morphology::~Morphology() {
		//dtor
	}

	void Morphology::addSiteType(const char site_type) {
		// check to make sure site type has not already been added
		for (int n = 0; n < (int)Site_types.size(); n++) {
			if (Site_types[n] == site_type) {
				return;
			}
		}
		Site_types.push_back(site_type);
		Site_type_counts.push_back(0);
		Mix_fractions.push_back(-1);
		vector<double> default_data(1, 0.0);
		Correlation_data.push_back(default_data);
		Tortuosity_data.push_back(default_data);
		vector<pair<double, int>> default_pairs(1, make_pair(0.0, 0));
		InterfacialHistogram_data.push_back(default_pairs);
		Domain_anisotropy_updated.push_back(false);
		Domain_sizes.push_back(-1);
		Domain_anisotropies.push_back(-1);
		Island_volume.push_back(-1);
	}

	double Morphology::calculateAdditionalEnergyChange(const long int site_index_main, const long int site_index_neighbor, const int growth_direction, const double additional_interaction) const {
		int x1, y1, z1, x2, y2, z2;
		int dx, dy, dz;
		int total_sites = 0;
		int count1_i = 0;
		int count2_i = 0;
		int count1_f = 0;
		int count2_f = 0;
		char site1_type, site2_type;
		Coords coords_main = lattice.getSiteCoords(site_index_main);
		x1 = coords_main.x;
		y1 = coords_main.y;
		z1 = coords_main.z;
		site1_type = lattice.getSiteType(coords_main);
		Coords coords_neighbor = lattice.getSiteCoords(site_index_neighbor);
		x2 = coords_neighbor.x;
		y2 = coords_neighbor.y;
		z2 = coords_neighbor.z;
		site2_type = lattice.getSiteType(coords_neighbor);
		switch (growth_direction) {
		case 1: // x-direction
			total_sites = 2;
			for (int i = -1; i <= 1; i += 2) {
				dx = lattice.calculateDX(x1, i);
				// Count the number of similar neighbors
				if (lattice.getSiteType(x1 + i + dx, y1, z1) == site1_type) {
					count1_i++;
				}
			}
			count1_f = total_sites - count1_i;
			for (int i = -1; i <= 1; i += 2) {
				dx = lattice.calculateDX(x2, i);
				// Count the number of similar neighbors
				if (lattice.getSiteType(x2 + i + dx, y2, z2) == site2_type) {
					count2_i++;
				}
			}
			count2_f = total_sites - count2_i;
			break;
		case 2: // y-direction
			total_sites = 2;
			for (int j = -1; j <= 1; j += 2) {
				dy = lattice.calculateDY(y1, j);
				// Count the number of similar neighbors
				if (lattice.getSiteType(x1, y1 + j + dy, z1) == site1_type) {
					count1_i++;
				}
			}
			count1_f = total_sites - count1_i;
			for (int j = -1; j <= 1; j += 2) {
				dy = lattice.calculateDY(y2, j);
				// Count the number of similar neighbors
				if (lattice.getSiteType(x2, y2 + j + dy, z2) == site2_type) {
					count2_i++;
				}
			}
			count2_f = total_sites - count2_i;
			break;
		case 3: // z-direction
			total_sites = 2;
			for (int k = -1; k <= 1; k += 2) {
				if (!lattice.isZPeriodic()) {
					if (z1 + k >= lattice.getHeight() || z1 + k < 0) { // Check for z boundary
						total_sites--;
						continue;
					}
				}
				dz = lattice.calculateDZ(z1, k);
				// Count the number of similar neighbors
				if (lattice.getSiteType(x1, y1, z1 + k + dz) == site1_type) {
					count1_i++;
				}
			}
			count1_f = total_sites - count1_i;
			for (int k = -1; k <= 1; k += 2) {
				if (!lattice.isZPeriodic()) {
					if (z2 + k >= lattice.getHeight() || z2 + k < 0) { // Check for z boundary
						total_sites--;
						continue;
					}
				}
				dz = lattice.calculateDZ(z2, k);
				// Count the number of similar neighbors
				if (lattice.getSiteType(x2, y2, z2 + k + dz) == site2_type) {
					count2_i++;
				}
			}
			count2_f = total_sites - count2_i;
			break;
		default:
			cout << "Error calculating the additional energy for the preferential growth direction!" << endl;
			break;
		}
		return -additional_interaction * ((count1_f - count1_i) + (count2_f - count2_i));
	}

	void Morphology::calculateAnisotropies() {
		cout << ID << ": Calculating the domain anisotropy..." << endl;
		// Select sites for correlation function calculation.
		// Site indices for each selected site are stored in the correlation_sites_data vector.
		vector<vector<long int>> correlation_sites_data(Site_types.size());
		for (int n = 0; n < (int)Site_types.size(); n++) {
			if ((int)correlation_sites_data[n].size() == 0) {
				// Only N_sampling_max sites are randomly selected
				getSiteSampling(correlation_sites_data[n], Site_types[n], Params.N_sampling_max);
			}
		}
		Domain_anisotropy_updated.assign(Site_types.size(), false);
		bool success = false;
		int cutoff_distance = 3;
		while (!success) {
			if (2 * cutoff_distance > lattice.getLength() || 2 * cutoff_distance > lattice.getWidth() || 2 * cutoff_distance > lattice.getHeight()) {
				success = false;
				break;
			}
			for (int i = 0; i < (int)Site_types.size(); i++) {
				// Only perform anisotropy calculations for site types that have not yet been updated and that have at least 100 site counts.
				if (!Domain_anisotropy_updated[i] && Site_type_counts[i] > 100) {
					cout << ID << ": Performing sampling anisotropy calculation with " << (int)correlation_sites_data[i].size() << " sites for site type " << (int)Site_types[i] << " with a cutoff of " << cutoff_distance << "..." << endl;
					Domain_anisotropy_updated[i] = calculateAnisotropy(correlation_sites_data[i], Site_types[i], cutoff_distance);
				}
			}
			// Check if the anisotropy has been successfully calculated for all sites types
			for (int i = 0; i < (int)Site_types.size(); i++) {
				// If the anisotropy was not updated for any of the site types, increment the cutoff_distance, break the for loop, and repeat the anisotropy calculations
				if (!Domain_anisotropy_updated[i] && Site_type_counts[i] > 100) {
					success = false;
					cutoff_distance++;
					break;
				}
				// If the i index reaches its max value here, then the anisotropy calculation was successful for all site types.
				if (i == (int)Site_types.size() - 1) {
					success = true;
				}
			}
		}
		if (!success) {
			cout << ID << ": Warning! Could not calculate the domain anisotropy." << endl;
		}
	}

	bool Morphology::calculateAnisotropy(const vector<long int>& correlation_sites, const char site_type, const int cutoff_distance) {
		int type_index = getSiteTypeIndex(site_type);
		int N_sites = 0;
		double correlation_length_x = 0;
		double correlation_length_y = 0;
		double correlation_length_z = 0;
		double d1, y1, y2, slope, intercept;
		Coords site_coords, coords_dest;
		vector<double> correlation_x(cutoff_distance + 1, 0.0);
		vector<double> correlation_y(cutoff_distance + 1, 0.0);
		vector<double> correlation_z(cutoff_distance + 1, 0.0);
		vector<int> site_count(cutoff_distance + 1, 0);
		vector<int> site_total(cutoff_distance + 1, 0);
		// Check that correlation sites vector is not empty
		if (!(correlation_sites.size() > 0)) {
			cout << ID << ": Error! Vector of site tags to be used in the anisotropy calculation is empty." << endl;
			throw invalid_argument("Error! Vector of site tags to be used in the anisotropy calculation is empty.");
		}
		for (int m = 0; m < (int)correlation_sites.size(); m++) {
			if (lattice.getSiteType(correlation_sites[m]) != site_type) {
				continue;
			}
			site_coords = lattice.getSiteCoords(correlation_sites[m]);
			// Calculate correlation length in the x-direction
			site_count.assign(cutoff_distance + 1, 0);
			for (int i = -cutoff_distance; i <= cutoff_distance; i++) {
				if (!lattice.checkMoveValidity(site_coords, i, 0, 0)) {
					continue;
				}
				lattice.calculateDestinationCoords(site_coords, i, 0, 0, coords_dest);
				if (lattice.getSiteType(site_coords) == lattice.getSiteType(coords_dest)) {
					site_count[abs(i)]++;
				}
			}
			for (int n = 1; n <= cutoff_distance; n++) {
				correlation_x[n] += (double)site_count[n] / 2;
			}
			// Calculate correlation length in the y-direction
			site_count.assign(cutoff_distance + 1, 0);
			for (int j = -cutoff_distance; j <= cutoff_distance; j++) {
				if (!lattice.checkMoveValidity(site_coords, 0, j, 0)) {
					continue;
				}
				lattice.calculateDestinationCoords(site_coords, 0, j, 0, coords_dest);
				if (lattice.getSiteType(site_coords) == lattice.getSiteType(coords_dest)) {
					site_count[abs(j)]++;
				}
			}
			for (int n = 1; n <= cutoff_distance; n++) {
				correlation_y[n] += (double)site_count[n] / 2;
			}
			// Calculate correlation length in the z-direction
			site_total.assign(cutoff_distance + 1, 0);
			site_count.assign(cutoff_distance + 1, 0);
			for (int k = -cutoff_distance; k <= cutoff_distance; k++) {
				if (!lattice.checkMoveValidity(site_coords, 0, 0, k)) {
					continue;
				}
				lattice.calculateDestinationCoords(site_coords, 0, 0, k, coords_dest);
				if (lattice.getSiteType(site_coords) == lattice.getSiteType(coords_dest)) {
					site_count[abs(k)]++;
				}
				site_total[abs(k)]++;
			}
			for (int n = 1; n <= cutoff_distance; n++) {
				if (site_total[n] > 0) {
					correlation_z[n] += (double)site_count[n] / site_total[n];
				}
			}
			N_sites++;
		}
		// Average correlation data over all starting sites and normalize
		double averaging = 1.0 / N_sites;
		double norm = 1.0 / (1.0 - Mix_fractions[type_index]);
		correlation_x[0] = 1.0;
		correlation_y[0] = 1.0;
		correlation_z[0] = 1.0;
		for (int n = 1; n <= cutoff_distance; n++) {
			correlation_x[n] *= averaging;
			correlation_x[n] -= Mix_fractions[type_index];
			correlation_x[n] *= norm;
			correlation_y[n] *= averaging;
			correlation_y[n] -= Mix_fractions[type_index];
			correlation_y[n] *= norm;
			correlation_z[n] *= averaging;
			correlation_z[n] -= Mix_fractions[type_index];
			correlation_z[n] *= norm;
		}
		// Find the bounds of where the pair-pair correlation functions reach 1/e
		bool success_x = false;
		bool success_y = false;
		bool success_z = false;
		for (int n = 1; n <= cutoff_distance; n++) {
			if (!success_x && correlation_x[n] < (1.0 / exp(1.0))) {
				d1 = n - 1;
				y1 = correlation_x[n - 1];
				y2 = correlation_x[n];
				// Use linear interpolation to determine the cross-over point
				slope = (y2 - y1);
				intercept = y1 - slope * d1;
				correlation_length_x = 2.0 * (1.0 / exp(1.0) - intercept) / slope;
				success_x = true;
			}
			if (!success_y && correlation_y[n] < (1.0 / exp(1.0))) {
				d1 = n - 1;
				y1 = correlation_y[n - 1];
				y2 = correlation_y[n];
				// Use linear interpolation to determine the cross-over point
				slope = (y2 - y1);
				intercept = y1 - slope * d1;
				correlation_length_y = 2.0 * (1.0 / exp(1.0) - intercept) / slope;
				success_y = true;
			}
			if (!success_z && correlation_z[n] < (1.0 / exp(1.0))) {
				d1 = n - 1;
				y1 = correlation_z[n - 1];
				y2 = correlation_z[n];
				// Use linear interpolation to determine the cross-over point
				slope = (y2 - y1);
				intercept = y1 - slope * d1;
				correlation_length_z = 2.0 * (1.0 / exp(1.0) - intercept) / slope;
				success_z = true;
			}
			else if (!success_z && cutoff_distance > lattice.getHeight()) {
				correlation_length_z = lattice.getHeight();
				success_z = true;
			}
		}
		if (!success_x || !success_y || !success_z) {
			cout << ID << ": Cutoff distance of " << cutoff_distance << " is too small to calculate anisotropy of domain type " << (int)site_type << "." << endl;
			Domain_anisotropies[type_index] = -1;
			return false;
		}
		if (4 * correlation_length_x > lattice.getLength() || 4 * correlation_length_y > lattice.getWidth()) {
			cout << "Warning.  Correlation length in x- or y-direction is greater than L/4." << endl;
			cout << "x-direction correlation length is " << correlation_length_x << "." << endl;
			cout << "y-direction correlation length is " << correlation_length_y << "." << endl;
		}
		Domain_anisotropies[type_index] = (2 * correlation_length_z) / (correlation_length_x + correlation_length_y);
		return true;
	}

	double Morphology::calculateCorrelationDistance(const vector<long int>& correlation_sites, vector<double>& correlation_data, const double mix_fraction, const int cutoff_distance) {
		vector<int> site_count, total_count;
		double distance;
		int bin;
		double d1, y1, y2, slope, intercept;
		Coords site_coords, coords_dest;
		if (cutoff_distance > lattice.getLength() || cutoff_distance > lattice.getWidth()) {
			cout << ID << ": Error, cutoff distance is greater than the lattice length and/or width." << endl;
			return -1;
		}
		// Resolution of correlation distance data is 0.5 lattice units
		int correlation_size_old = (int)correlation_data.size();
		int correlation_size_new = 2 * cutoff_distance + 1;
		if (correlation_size_old >= correlation_size_new) {
			cout << ID << ": Error, new cutoff distance is not greater than the previous cutoff distance and no new calculations have been performed." << endl;
			return -1;
		}
		// Initialize vectors to store correlation function data
		for (int m = 0; m < (correlation_size_new - correlation_size_old); m++) {
			correlation_data.push_back(0);
		}
		// Loop through all selected sites and determine the correlation function for each
		// The pair-pair correlation is determined based on the fraction of sites that are the same as the starting site and this function is calculated as a function of distance from the starting site.
		// Sites surrounding the start site are placed into bins based on their distance from the starting site.
		// Bins covering a distance range of half a lattice unit are used, ex: second bin is from 0.25a to 0.7499a, third bin is from 0.75a to 1.2499a, etc.
		// site_count vector stores the number of sites that are are the same type as the starting site for each bin
		// total_count vector stores the total number of sites in each bin
		site_count.assign(2 * cutoff_distance + 1, 0);
		total_count.assign(2 * cutoff_distance + 1, 0);
		for (int m = 0; m < (int)correlation_sites.size(); m++) {
			site_coords = lattice.getSiteCoords(correlation_sites[m]);
			fill(site_count.begin(), site_count.end(), 0);
			fill(total_count.begin(), total_count.end(), 0);
			for (int i = -cutoff_distance; i <= cutoff_distance; i++) {
				for (int j = -cutoff_distance; j <= cutoff_distance; j++) {
					for (int k = -cutoff_distance; k <= cutoff_distance; k++) {
						// The distance between two sites is rounded to the nearest half a lattice unit
						bin = round_int(2.0 * sqrt(i*i + j * j + k * k));
						// Calculation is skipped for bin values that have already been calculated during previous calls to the calculateCorrelationDistance function
						if (bin < (correlation_size_old - 1)) {
							continue;
						}
						distance = (double)bin / 2.0;
						if (distance > cutoff_distance) {
							continue;
						}
						if (!lattice.checkMoveValidity(site_coords, i, j, k)) {
							continue;
						}
						lattice.calculateDestinationCoords(site_coords, i, j, k, coords_dest);
						if (lattice.getSiteType(site_coords) == lattice.getSiteType(coords_dest)) {
							site_count[bin]++;
						}
						total_count[bin]++;
					}
				}
			}
			//  Calculate the fraction of similar sites for each bin
			for (int n = 0; n < (int)correlation_data.size(); n++) {
				if (n < correlation_size_old) {
					continue;
				}
				if (total_count[n] > 0) {
					correlation_data[n] += (double)site_count[n] / (double)total_count[n];
				}
				else {
					correlation_data[n] += 1;
				}
			}
		}
		// Average overall starting sites and normalize the correlation data
		double averaging = 1.0 / correlation_sites.size();
		double norm = 1.0 / (1.0 - mix_fraction);
		for (int n = 0; n < (int)correlation_data.size(); n++) {
			if (n < correlation_size_old) {
				continue;
			}
			correlation_data[n] *= averaging;
			correlation_data[n] -= mix_fraction;
			correlation_data[n] *= norm;
		}
		// Find the bounds of where the pair-pair correlation function first crosses over the Mix_fraction
		if (Params.Enable_mix_frac_method) {
			for (int n = 2; n < (int)correlation_data.size(); n++) {
				if (correlation_data[n] < 0) {
					d1 = (double)(n - 1) * 0.5;
					y1 = correlation_data[n - 1];
					y2 = correlation_data[n];
					// Use linear interpolation to determine the cross-over point
					slope = (y2 - y1) * 2.0;
					intercept = y1 - slope * d1;
					return -intercept / slope;
				}
				if (correlation_data[n] > correlation_data[n - 1]) {
					return (double)(n - 1) / 2.0;
				}
			}
		}
		// Find the bounds of where the pair-pair correlation function first reaches within 1/e of the Mix_fraction
		if (Params.Enable_e_method) {
			for (int n = 2; n < (int)correlation_data.size(); n++) {
				if (correlation_data[n] < (1.0 / exp(1.0))) {
					d1 = (double)(n - 1) * 0.5;
					y1 = correlation_data[n - 1];
					y2 = correlation_data[n];
					// Use linear interpolation to determine the cross-over point
					slope = (y2 - y1) * 2.0;
					intercept = y1 - slope * d1;
					return 2.0 * (1.0 / exp(1.0) - intercept) / slope;
				}
			}
		}
		return -1;
	}

	void Morphology::calculateCorrelationDistances() {
		if (Params.Enable_extended_correlation_calc) {
			cout << ID << ": Calculating the domain size using the extended pair-pair correlation function using a cutoff radius of " << Params.Extended_correlation_cutoff_distance << "..." << endl;
		}
		if (Params.Enable_mix_frac_method) {
			cout << ID << ": Calculating the domain size from the pair-pair correlation function using the mix fraction method..." << endl;
		}
		else if (Params.Enable_e_method) {
			cout << ID << ": Calculating the domain size from the pair-pair correlation function using the 1/e method..." << endl;
		}
		vector<vector<long int>> correlation_sites_data(Site_types.size());
		for (int n = 0; n < (int)Site_types.size(); n++) {
			// Select sites for correlation function calculation.
			// Site indices for each selected site are stored in the Correlation_sites vector.
			if ((int)correlation_sites_data[n].size() == 0) {
				getSiteSampling(correlation_sites_data[n], Site_types[n], Params.N_sampling_max);
			}
			// Clear the current Correlation_data
			Correlation_data[n].clear();
		}
		vector<bool> domain_size_updated(Site_types.size(), false);
		int cutoff_distance;
		double domain_size;
		for (int n = 0; n < (int)Site_types.size(); n++) {
			if (Params.Enable_extended_correlation_calc) {
				cutoff_distance = Params.Extended_correlation_cutoff_distance;
			}
			else {
				cutoff_distance = 3;
			}
			domain_size = -1;
			// The correlation function calculation is called with an increasing cutoff distance until successful.
			while (!domain_size_updated[n]) {
				if (2 * cutoff_distance > lattice.getLength() || 2 * cutoff_distance > lattice.getWidth() || (lattice.isZPeriodic() && 2 * cutoff_distance > lattice.getHeight())) {
					cout << ID << ": Correlation calculation cutoff radius of " << cutoff_distance << " is now too large to continue accurately calculating the correlation function for site type " << (int)Site_types[n] << "." << endl;
					break;
				}
				if (Site_type_counts[n] > 100) {
					cout << ID << ": Performing sampling domain size calculation with " << (int)correlation_sites_data[n].size() << " sites for site type " << (int)Site_types[n] << " with a cutoff radius of " << cutoff_distance << "..." << endl;
					domain_size = calculateCorrelationDistance(correlation_sites_data[n], Correlation_data[n], Mix_fractions[n], cutoff_distance);
				}
				if (domain_size > 0) {
					domain_size_updated[n] = true;
					Domain_sizes[n] = domain_size;
				}
				else {
					cout << ID << ": Cutoff distance of " << cutoff_distance << " is too small to calculate the size of domain type " << (int)Site_types[n] << "." << endl;
					cutoff_distance++;
				}
			}
		}
	}

	void Morphology::calculateDepthDependentData() {
		// Save current status of extended correlation calculation option
		bool Enable_extended_correlation_calc_old = Params.Enable_extended_correlation_calc;
		// Temporarily disable extended correlation calculation
		Params.Enable_extended_correlation_calc = false;
		if (Params.Enable_mix_frac_method) {
			cout << ID << ": Calculating the depth dependent domain size from the pair-pair correlation function using the mix fraction method..." << endl;
		}
		else if (Params.Enable_e_method) {
			cout << ID << ": Calculating the depth dependent domain size from the pair-pair correlation function using the 1/e method..." << endl;
		}
		vector<vector<long int>> correlation_sites_data(Site_types.size());
		vector<double> correlation_data;
		vector<bool> domain_size_updated(Site_types.size(), false);
		Depth_composition_data.assign(Site_types.size(), vector<double>(lattice.getHeight(), 0.0));
		Depth_domain_size_data.assign(Site_types.size(), vector<double>(lattice.getHeight(), 0.0));
		Depth_iv_data.assign(lattice.getHeight(), 0.0);
		vector<int> counts(Site_types.size(), 0);
		int site_count;
		double mix_fraction_local;
		Coords coords, coords_dest;
		int counts_total = 0;
		int cutoff_distance;
		double domain_size;
		for (int z = 0; z < lattice.getHeight(); z++) {
			//cout << ID << ": Calculating depth dependent data for z = " << z << endl;
			// Calculate depth dependent composition
			counts.assign(Site_types.size(), 0);
			for (int x = 0; x < lattice.getLength(); x++) {
				for (int y = 0; y < lattice.getWidth(); y++) {
					counts[getSiteTypeIndex(lattice.getSiteType(x, y, z))]++;
				}
			}
			for (int n = 0; n < (int)Site_types.size(); n++) {
				Depth_composition_data[n][z] = (double)counts[n] / (double)(lattice.getLength()*lattice.getWidth());
			}
			// Calculate depth dependent interfacial volume fraction
			site_count = 0;
			for (int x = 0; x < lattice.getLength(); x++) {
				for (int y = 0; y < lattice.getWidth(); y++) {
					coords.setXYZ(x, y, z);
					if (isNearInterface(coords, 1.0)) {
						site_count++;
					}
				}
			}
			Depth_iv_data[z] = (double)site_count / (double)(lattice.getLength()*lattice.getWidth());
			// Select sites for correlation function calculation.
			correlation_sites_data.assign(Site_types.size(), vector<long int>(0));
			for (int n = 0; n < (int)Site_types.size(); n++) {
				if ((int)correlation_sites_data[n].size() == 0) {
					getSiteSamplingZ(correlation_sites_data[n], Site_types[n], Params.N_sampling_max, z);
				}
			}
			// Calculate depth dependent domain size
			domain_size_updated.assign(Site_types.size(), false);
			for (int n = 0; n < (int)Site_types.size(); n++) {
				cutoff_distance = 5;
				correlation_data.clear();
				domain_size = -1;
				// The correlation function calculation is called with an increasing cutoff distance until successful.
				while (!domain_size_updated[n]) {
					if (2 * cutoff_distance > lattice.getLength() || 2 * cutoff_distance > lattice.getWidth() || 2 * cutoff_distance > lattice.getHeight()) {
						//cout << ID << ": Correlation calculation cutoff radius is now too large to continue accurately calculating the correlation function for site type " << (int)Site_types[n] << "." << endl;
						break;
					}
					// Calculate local mix fraction
					site_count = 0;
					counts_total = 0;
					for (int x = 0; x < lattice.getLength(); x++) {
						for (int y = 0; y < lattice.getWidth(); y++) {
							for (int k = -cutoff_distance; k <= cutoff_distance; k++) {
								coords.setXYZ(x, y, z);
								if (!lattice.checkMoveValidity(coords, 0, 0, k)) {
									continue;
								}
								lattice.calculateDestinationCoords(coords, 0, 0, k, coords_dest);
								if (lattice.getSiteType(coords_dest) == Site_types[n]) {
									site_count++;
								}
								counts_total++;
							}
						}
					}
					mix_fraction_local = (double)site_count / (double)counts_total;
					if (Site_type_counts[n] > 10) {
						//cout << ID << ": Performing sampling domain size calculation with " << (int)correlation_sites_data[n].size() << " sites for site type " << (int)Site_types[n] << " with a cutoff radius of " << cutoff_distance << "..." << endl;
						domain_size = calculateCorrelationDistance(correlation_sites_data[n], correlation_data, mix_fraction_local, cutoff_distance);
					}
					if (domain_size > 0) {
						Depth_domain_size_data[n][z] = domain_size;
						domain_size_updated[n] = true;
					}
					else {
						cutoff_distance++;
					}
				}
			}
			// Calculate depth dependent anisotropy
		}
		// Reset extended correlation calculation option to original value
		Params.Enable_extended_correlation_calc = Enable_extended_correlation_calc_old;
	}

	double Morphology::calculateDissimilarFraction(const Coords& coords, const int rescale_factor) const {
		int site_count = 0;
		int count_dissimilar = 0;
		Coords coords_dest;
		// When the rescale factor is 1, the radius is 1, and the radius increases for larger rescale factors.
		int radius = (rescale_factor <= 2) ? 1 : (int)ceil((double)(rescale_factor + 1) / 2);
		int cutoff_squared = (rescale_factor <= 2) ? 2 : (int)floor(intpow((rescale_factor + 1.0) / 2.0, 2));
		for (int i = -radius; i <= radius; i++) {
			for (int j = -radius; j <= radius; j++) {
				for (int k = -radius; k <= radius; k++) {
					if ((i*i + j * j + k * k) > cutoff_squared) {
						continue;
					}
					if (!lattice.checkMoveValidity(coords, i, j, k)) {
						continue;
					}
					lattice.calculateDestinationCoords(coords, i, j, k, coords_dest);
					if (lattice.getSiteType(coords) != lattice.getSiteType(coords_dest)) {
						count_dissimilar++;
					}
					site_count++;
				}
			}
		}
		return (double)count_dissimilar / (double)site_count;
	}

	double Morphology::calculateEnergyChangeSimple(const long int site_index1, const long int site_index2, const double interaction_energy1, const double interaction_energy2) {
		// Used with bond formation algorithm
		static const double one_over_sqrt2 = 1 / sqrt(2);
		//static const double one_over_sqrt3 = 1 / sqrt(3);
		char sum1_1_delta;
		char sum2_1_delta;
		//char sum3_1_delta;
		char sum1_2_delta;
		char sum2_2_delta;
		//char sum3_2_delta;
		double sum_1_delta, sum_2_delta;
		char site1_type = lattice.getSiteType(site_index1);
		// Calculate change around site 1
		char sum1_1i = Neighbor_counts[site_index1].sum1;
		char sum2_1i = Neighbor_counts[site_index1].sum2;
		char sum3_1i = Neighbor_counts[site_index1].sum3;
		char sum1_2f = Neighbor_info[site_index1].total1 - sum1_1i - 1;
		char sum2_2f = Neighbor_info[site_index1].total2 - sum2_1i;
		char sum3_2f = Neighbor_info[site_index1].total3 - sum3_1i;
		// Calculate change around site 2
		char sum1_2i = Neighbor_counts[site_index2].sum1;
		char sum2_2i = Neighbor_counts[site_index2].sum2;
		char sum3_2i = Neighbor_counts[site_index2].sum3;
		char sum1_1f = Neighbor_info[site_index2].total1 - sum1_2i - 1;
		char sum2_1f = Neighbor_info[site_index2].total2 - sum2_2i;
		char sum3_1f = Neighbor_info[site_index2].total3 - sum3_2i;
		// Save swapped state into temp_counts1 and temp_counts2
		Temp_counts1.sum1 = sum1_2f;
		Temp_counts1.sum2 = sum2_2f;
		Temp_counts1.sum3 = sum3_2f;
		Temp_counts2.sum1 = sum1_1f;
		Temp_counts2.sum2 = sum2_1f;
		Temp_counts2.sum3 = sum3_1f;
		// Calculate change
		sum1_1_delta = sum1_1f - sum1_1i;
		sum2_1_delta = sum2_1f - sum2_1i;
		//sum3_1_delta = sum3_1f - sum3_1i;
		sum1_2_delta = sum1_2f - sum1_2i;
		sum2_2_delta = sum2_2f - sum2_2i;
		//sum3_2_delta = sum3_2f - sum3_2i;
		sum_1_delta = -(double)sum1_1_delta - (double)sum2_1_delta*one_over_sqrt2;
		sum_2_delta = -(double)sum1_2_delta - (double)sum2_2_delta*one_over_sqrt2;
		// By default interactions with the third-nearest neighbors are not included, but when enabled they are added here
		//if (Params.Enable_third_neighbor_interaction) {
		//	sum_1_delta -= (double)sum3_1_delta*one_over_sqrt3;
		//	sum_2_delta -= (double)sum3_2_delta*one_over_sqrt3;
		//}
		if (site1_type == 1) {
			return interaction_energy1 * sum_1_delta + interaction_energy2 * sum_2_delta;
		}
		else {
			return interaction_energy2 * sum_1_delta + interaction_energy1 * sum_2_delta;
		}
	}

	//double Morphology::calculateEnergyChange(const Coords& coords1, const Coords& coords2, const double interaction_energy1, const double interaction_energy2) const {
	//	// Used with bond formation algorithm
	//	char site1_type, site2_type;
	//	int sum1_1_delta, sum2_1_delta, sum3_1_delta, sum1_2_delta, sum2_2_delta, sum3_2_delta;
	//	double sum_1_delta, sum_2_delta;
	//	static const double one_over_sqrt2 = 1 / sqrt(2);
	//	static const double one_over_sqrt3 = 1 / sqrt(3);
	//	Coords coords_dest;
	//	int sum1_1i = 0;
	//	int sum2_1i = 0;
	//	int sum3_1i = 0;
	//	int sum1_2i = 0;
	//	int sum2_2i = 0;
	//	int sum3_2i = 0;
	//	int sum1_1f = 0;
	//	int sum2_1f = 0;
	//	int sum3_1f = 0;
	//	int sum1_2f = 0;
	//	int sum2_2f = 0;
	//	int sum3_2f = 0;
	//	// There are in total 6 first-nearest, 12 second-nearest, and 8 third-nearest neighbors
	//	int total1 = 6;
	//	int total2 = 12;
	//	int total3 = 8;
	//	// Calculate change around x1,y1,z1
	//	site1_type = lattice.getSiteType(coords1);
	//	for (int i = -1; i <= 1; i++) {
	//		for (int j = -1; j <= 1; j++) {
	//			for (int k = -1; k <= 1; k++) {
	//				if (!lattice.checkMoveValidity(coords1, i, j, k)) {
	//					// Total site counts must be reduced if next to a hard boundary
	//					switch (i*i + j * j + k * k) {
	//					case 1:
	//						total1--;
	//						break;
	//					case 2:
	//						total2--;
	//						break;
	//					case 3:
	//						total3--;
	//						break;
	//					default:
	//						break;
	//					}
	//					continue;
	//				}
	//				lattice.calculateDestinationCoords(coords1, i, j, k, coords_dest);
	//				// Count the number of similar neighbors
	//				if (lattice.getSiteType(coords_dest) == site1_type) {
	//					switch (i*i + j * j + k * k) {
	//					case 1:
	//						sum1_1i++;
	//						break;
	//					case 2:
	//						sum2_1i++;
	//						break;
	//					case 3:
	//						sum3_1i++;
	//						break;
	//					default:
	//						break;
	//					}
	//				}
	//			}
	//		}
	//	}
	//	sum1_2f = total1 - sum1_1i - 1;
	//	sum2_2f = total2 - sum2_1i;
	//	sum3_2f = total3 - sum3_1i;
	//	// Calculate change around x2,y2,z2
	//	site2_type = lattice.getSiteType(coords2);
	//	// There are in total 6 first-nearest, 12 second-nearest, and 8 third-nearest neighbors
	//	total1 = 6;
	//	total2 = 12;
	//	total3 = 8;
	//	for (int i = -1; i <= 1; i++) {
	//		for (int j = -1; j <= 1; j++) {
	//			for (int k = -1; k <= 1; k++) {
	//				if (!lattice.checkMoveValidity(coords2, i, j, k)) {
	//					switch (i*i + j * j + k * k) {
	//					case 1:
	//						total1--;
	//						break;
	//					case 2:
	//						total2--;
	//						break;
	//					case 3:
	//						total3--;
	//						break;
	//					default:
	//						break;
	//					}
	//					continue;
	//				}
	//				lattice.calculateDestinationCoords(coords2, i, j, k, coords_dest);
	//				// Count the number of similar neighbors
	//				if (lattice.getSiteType(coords_dest) == site2_type) {
	//					switch (i*i + j * j + k * k) {
	//					case 1:
	//						sum1_2i++;
	//						break;
	//					case 2:
	//						sum2_2i++;
	//						break;
	//					case 3:
	//						sum3_2i++;
	//						break;
	//					default:
	//						break;
	//					}
	//				}
	//			}
	//		}
	//	}
	//	sum1_1f = total1 - sum1_2i - 1;
	//	sum2_1f = total2 - sum2_2i;
	//	sum3_1f = total3 - sum3_2i;
	//	sum1_1_delta = sum1_1f - sum1_1i;
	//	sum2_1_delta = sum2_1f - sum2_1i;
	//	sum3_1_delta = sum3_1f - sum3_1i;
	//	sum1_2_delta = sum1_2f - sum1_2i;
	//	sum2_2_delta = sum2_2f - sum2_2i;
	//	sum3_2_delta = sum3_2f - sum3_2i;
	//	sum_1_delta = -(double)sum1_1_delta - (double)sum2_1_delta*one_over_sqrt2;
	//	sum_2_delta = -(double)sum1_2_delta - (double)sum2_2_delta*one_over_sqrt2;
	//	// By default interactions with the third-nearest neighbors are not included, but when enabled they are added here
	//	if (Enable_third_neighbor_interaction) {
	//		sum_1_delta -= (double)sum3_1_delta*one_over_sqrt3;
	//		sum_2_delta -= (double)sum3_2_delta*one_over_sqrt3;
	//	}
	//	if (site1_type == (char)1) {
	//		return interaction_energy1 * sum_1_delta + interaction_energy2 * sum_2_delta;
	//	}
	//	else {
	//		return interaction_energy2 * sum_1_delta + interaction_energy1 * sum_2_delta;
	//	}
	//}

	double Morphology::calculateInterfacialAreaVolumeRatio() const {
		unsigned long site_face_count = 0;
		Coords coords, coords_dest;
		for (int m = 0; m < (int)Site_types.size() - 1; m++) {
			for (int n = m + 1; n < (int)Site_types.size(); n++) {
				for (int x = 0; x < lattice.getLength(); x++) {
					for (int y = 0; y < lattice.getWidth(); y++) {
						for (int z = 0; z < lattice.getHeight(); z++) {
							coords.setXYZ(x, y, z);
							if (lattice.getSiteType(coords) == Site_types[m]) {
								for (int i = -1; i <= 1; i++) {
									for (int j = -1; j <= 1; j++) {
										for (int k = -1; k <= 1; k++) {
											if (abs(i) + abs(j) + abs(k) > 1) {
												continue;
											}
											if (!lattice.checkMoveValidity(coords, i, j, k)) {
												continue;
											}
											lattice.calculateDestinationCoords(coords, i, j, k, coords_dest);
											if (lattice.getSiteType(coords_dest) == Site_types[n]) {
												site_face_count++;
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
		return (double)site_face_count / (double)lattice.getNumSites();
	}

	void Morphology::calculateInterfacialDistanceHistogram() {
		Coords coords, coords_dest;
		float d;
		float d_temp;
		// The shortest distance from each site to the interface is stored in the path_distances vector
		vector<float> path_distances;
		path_distances.assign(lattice.getNumSites(), 0);
		// Calculate distances to the interface by expanding outward from the interface
		// A first scan over the lattice is done to identify the sites at the interface, at a distance of less than 2 lattice units from the interface
		// Subsequent scans expand outward 1 lattice unit at a time from these interfacial sites until no sites are left.
		int calc_count = 1;
		float d_current = (float)1.99;
		while (calc_count > 0) {
			calc_count = 0;
			for (int x = 0; x < lattice.getLength(); x++) {
				for (int y = 0; y < lattice.getWidth(); y++) {
					for (int z = 0; z < lattice.getHeight(); z++) {
						coords.setXYZ(x, y, z);
						// Only perform the calculation on sites with a yet unknown interfacial distance
						if (path_distances[lattice.getSiteIndex(x, y, z)] < 0.1) {
							d = -1;
							// Look around at neighboring sites
							for (int i = -1; i <= 1; i++) {
								for (int j = -1; j <= 1; j++) {
									for (int k = -1; k <= 1; k++) {
										if (!lattice.checkMoveValidity(coords, i, j, k)) {
											continue;
										}
										lattice.calculateDestinationCoords(coords, i, j, k, coords_dest);
										// Initial scan identifies interfacial sites
										if (d_current < 2) {
											if (lattice.getSiteType(coords_dest) != lattice.getSiteType(x, y, z)) {
												d_temp = sqrt((float)(i*i + j * j + k * k));
												if (d < 0 || d_temp < d) {
													d = d_temp;
												}
											}
										}
										// Subsequent scans identify sites of the same type
										// A temporary distance to the interface from the target site by way of the identified neighbor site is calculated
										// The temporary distance is only stored if it is shorter than the previous temporary distance, ensuring that the shortest interfacial distance is calculated
										else if (lattice.getSiteType(coords_dest) == lattice.getSiteType(x, y, z) && path_distances[lattice.getSiteIndex(coords_dest)] > 0.1) {
											d_temp = path_distances[lattice.getSiteIndex(coords_dest)] + sqrt((float)(i*i + j * j + k * k));
											if (d < 0 || d_temp < d) {
												d = d_temp;
											}
										}
									}
								}
							}
							// The temporary distance is only accepted if it less than the expansion limit (d_current).
							if (d > 0 && d < d_current) {
								path_distances[lattice.getSiteIndex(x, y, z)] = d;
								calc_count++;
							}
						}
					}
				}
			}
			// Incrementing the expansion limit (d_current) one lattice unit at a time ensures that the shortest path to the interface is determined.
			d_current += 1;
		}
		// Clear existing interfacial distance histogram data
		for (int n = 0; n < (int)Site_types.size(); n++) {
			InterfacialHistogram_data[n].clear();
		}
		vector<int> counts((int)Site_types.size(), 0);
		// Gather path data into separate vectors for each site type
		vector<vector<int>> segmented_path_data(Site_types.size(), vector<int>());
		for (int m = 0; m < (int)path_distances.size(); m++) {
			for (int n = 0; n < (int)Site_types.size(); n++) {
				if (lattice.getSiteType(m) == Site_types[n]) {
					segmented_path_data[n].push_back(round_int(path_distances[m]));
				}
			}
		}
		// Path data is rounded to the nearest integer lattice unit
		for (int n = 0; n < (int)Site_types.size(); n++) {
			InterfacialHistogram_data[n] = calculateHist(segmented_path_data[n], 1);
		}
	}

	double Morphology::calculateInterfacialVolumeFraction() const {
		unsigned long site_count = 0;
		Coords coords;
		for (int x = 0; x < lattice.getLength(); x++) {
			for (int y = 0; y < lattice.getWidth(); y++) {
				for (int z = 0; z < lattice.getHeight(); z++) {
					coords.setXYZ(x, y, z);
					if (isNearInterface(coords, 1.0)) {
						site_count++;
					}
				}
			}
		}
		return (double)site_count / (double)lattice.getNumSites();
	}

	void Morphology::calculateMixFractions() {
		//Calculate final Mix_fraction
		vector<int> counts((int)Site_types.size(), 0);
		int type_index;
		for (int x = 0; x < lattice.getLength(); x++) {
			for (int y = 0; y < lattice.getWidth(); y++) {
				for (int z = 0; z < lattice.getHeight(); z++) {
					type_index = getSiteTypeIndex(lattice.getSiteType(x, y, z));
					counts[type_index]++;
				}
			}
		}
		for (int i = 0; i < (int)Site_types.size(); i++) {
			Mix_fractions[i] = (double)counts[i] / (double)lattice.getNumSites();
		}
	}

	Morphology::NeighborCounts Morphology::calculateNeighborCounts(const Coords& coords) const {
		Coords coords_dest;
		NeighborCounts counts;
		counts.sum1 = 0;
		counts.sum2 = 0;
		counts.sum3 = 0;
		// Calculate similar neighbors around x,y,z
		for (int i = -1; i <= 1; i++) {
			for (int j = -1; j <= 1; j++) {
				for (int k = -1; k <= 1; k++) {
					if (!lattice.checkMoveValidity(coords, i, j, k)) {
						continue;
					}
					lattice.calculateDestinationCoords(coords, i, j, k, coords_dest);
					// Count the number of similar neighbors
					if (lattice.getSiteType(coords_dest) == lattice.getSiteType(coords)) {
						switch (i*i + j * j + k * k) {
						case 1:
							counts.sum1++;
							break;
						case 2:
							counts.sum2++;
							break;
						case 3:
							counts.sum3++;
							break;
						default:
							break;
						}
					}
				}
			}
		}
		return counts;
	}

	bool Morphology::calculatePathDistances(vector<float>& path_distances) {
		Coords coords;
		long int current_index;
		long int neighbor_index;
		float d;
		float d_temp;
		const static float sqrt_two = sqrt((float)2.0);
		const static float sqrt_three = sqrt((float)3.0);
		// Create and initialize a blank node.
		// Each node contains a vector with indices of all first- ,second-, and third-nearest neighbors (at most 26 neighbors).
		// Another vector stores the squared distance to each of the neighbors.
		// Each node also has an estimated distance from the destination.
		Node temp_node;
		// Create a node vector that is the same size as the lattice and initialize with blank nodes.
		vector<Node> Node_vector;
		Node_vector.assign(lattice.getNumSites(), temp_node);
		// The neighbor_nodes set is sorted by the estimated distance of nodes in the set.
		// This set is used in Dijsktra's algorithm to keep a sorted list of all nodes that are neighboring nodes that have their path distances already determined.
		// Once the path distance for a particular node is fixed, all of its neighboring nodes that have not yet been fixed are added to the neighbor_nodes set.
		set<vector<Node>::iterator, NodeIteratorCompare> neighbor_nodes;
		vector<Node>::iterator current_it;
		set<vector<Node>::iterator>::iterator current_set_it;
		set<vector<Node>::iterator>::iterator set_it;
		// Determine node connectivity.
		for (int x = 0; x < lattice.getLength(); x++) {
			for (int y = 0; y < lattice.getWidth(); y++) {
				for (int z = 0; z < lattice.getHeight(); z++) {
					coords.setXYZ(x, y, z);
					createNode(temp_node, coords);
					Node_vector[lattice.getSiteIndex(x, y, z)] = temp_node;
				}
			}
		}
		// Initialize the path distances of top and bottom surfaces of the lattice.
		for (int x = 0; x < lattice.getLength(); x++) {
			for (int y = 0; y < lattice.getWidth(); y++) {
				if (lattice.getSiteType(x, y, 0) == Site_types[0]) {
					path_distances[lattice.getSiteIndex(x, y, 0)] = 1;
				}
				if (lattice.getSiteType(x, y, lattice.getHeight() - 1) == Site_types[1]) {
					path_distances[lattice.getSiteIndex(x, y, lattice.getHeight() - 1)] = 1;
				}
			}
		}
		// The pathfinding algorithm is performed for one domain type at a time.
		int z;
		for (int n = 0; n < 2; n++) {
			// Use Dijkstra's algorithm to fill in the remaining path distance data.
			cout << ID << ": Executing Dijkstra's algorithm to calculate shortest paths through domain type " << (int)Site_types[n] << ".\n";
			if (n == 0) {
				z = 1;
			}
			else {
				z = lattice.getHeight() - 2;
			}
			// Initialize the neighbor node set.
			for (int x = 0; x < lattice.getLength(); x++) {
				for (int y = 0; y < lattice.getWidth(); y++) {
					if (lattice.getSiteType(x, y, z) == Site_types[n]) {
						d = -1;
						for (int i = 0; i < 26; i++) {
							if (Node_vector[lattice.getSiteIndex(x, y, z)].neighbor_indices[i] < 0) {
								break;
							}
							if (path_distances[Node_vector[lattice.getSiteIndex(x, y, z)].neighbor_indices[i]] > 0) {
								d_temp = path_distances[Node_vector[lattice.getSiteIndex(x, y, z)].neighbor_indices[i]] + sqrt((float)(Node_vector[lattice.getSiteIndex(x, y, z)]).neighbor_distances_sq[i]);
								if (d < 0 || d_temp < d) {
									d = d_temp;
								}
							}
						}
						if (d > 0) {
							Node_vector[lattice.getSiteIndex(x, y, z)].distance_est = d;
							neighbor_nodes.insert(Node_vector.begin() + lattice.getSiteIndex(x, y, z));
						}
					}
				}
			}
			while (!neighbor_nodes.empty()) {
				// The neighbor nodes set is sorted, so the first node has the shortest estimated path distance and is set to the current node.
				current_set_it = neighbor_nodes.begin();
				current_it = *current_set_it;
				//current_index = (long)(current_it - Node_vector.begin());
				current_index = (long int)distance(Node_vector.begin(), current_it);
				// Insert neighbors of the current node into the neighbor node set.
				for (int i = 0; i < 26; i++) {
					if (Node_vector[current_index].neighbor_indices[i] < 0) {
						break;
					}
					neighbor_index = Node_vector[current_index].neighbor_indices[i];
					// Check that the target neighbor node has not already been finalized.
					if (path_distances[neighbor_index] > 0) {
						continue;
					}
					// Calculate the estimated path distance.
					//d = Node_vector[current_index].distance_est + sqrt((int)(Node_vector[current_index]).neighbor_distances_sq[i]);
					switch (Node_vector[current_index].neighbor_distances_sq[i]) {
					case 1:
						d = Node_vector[current_index].distance_est + 1;
						break;
					case 2:
						d = Node_vector[current_index].distance_est + sqrt_two;
						break;
					case 3:
						d = Node_vector[current_index].distance_est + sqrt_three;
						break;
					default:
						d = Node_vector[current_index].distance_est;
						break;
					}
					// Check if node is already in the neighbor node set.
					set_it = neighbor_nodes.find(Node_vector.begin() + neighbor_index);
					// If the node is not already in the list, update the distance estimate for the target neighbor node and insert the node into the set.
					if (set_it == neighbor_nodes.end()) {
						Node_vector[neighbor_index].distance_est = d;
						neighbor_nodes.insert(Node_vector.begin() + neighbor_index);
					}
					// If it already is in the list, replace it only if the new path distance estimate is smaller.
					else if (d < (*set_it)->distance_est) {
						neighbor_nodes.erase(set_it);
						Node_vector[neighbor_index].distance_est = d;
						neighbor_nodes.insert(Node_vector.begin() + neighbor_index);
					}
				}
				// Finalize the path distance of current node and remove the current node from the neighbor node set.
				path_distances[current_index] = Node_vector[current_index].distance_est;
				neighbor_nodes.erase(current_set_it);
			}
		}
		// Clear allocated memory for the neighbor nodes set
		set<vector<Node>::iterator, NodeIteratorCompare>().swap(neighbor_nodes);
		return true;
	}

	bool Morphology::calculatePathDistances_ReducedMemory(vector<float>& path_distances) {
		float d;
		float d_temp;
		Coords coords;
		const static float sqrt_two = sqrt((float)2.0);
		const static float sqrt_three = sqrt((float)3.0);
		// Create a temporary node to be used throughout the function.
		// Each node contains a vector with indices of all first- ,second-, and third-nearest neighbors (at most 26 neighbors).
		// Another vector stores the squared distance to each of the neighbors.
		// Each node also has an estimated distance from the destination and the site index.
		Node temp_node;
		// Create Node vector to store neighbor nodes
		// This vector is used in Dijsktra's algorithm to keep a list of all nodes that are neighboring sites that already have their path distances determined.
		// Once the path distance for a particular node is finalized, it is removed from the neighbor node vector and
		// all of its neighboring nodes that have not yet been finalized are added to the neighbor node vector.
		vector<Node> Node_vector;
		int Node_vector_count = 0;
		long int node_index = -1;
		long int current_index = -1;
		long int neighbor_index = -1;
		// Create a boolean vector that keeps track of whether or not nodes have already been added to the node vector
		vector<bool> added;
		added.assign(lattice.getNumSites(), false);
		int z;
		// The pathfinding algorithm is performed for one domain type at a time.
		for (int n = 0; n < 2; n++) {
			// Use Dijkstra's algorithm to fill in the remaining path distance data.
			cout << ID << ": Executing Dijkstra's algorithm to calculate shortest paths through domain type " << (int)Site_types[n] << "." << endl;
			// Clear Node vector
			Node_vector.clear();
			Node_vector.assign(lattice.getLength()*lattice.getWidth(), temp_node);
			// Initialize the path distances with known values
			for (int x = 0; x < lattice.getLength(); x++) {
				for (int y = 0; y < lattice.getWidth(); y++) {
					if (n == 0 && lattice.getSiteType(x, y, 0) == Site_types[n]) {
						path_distances[lattice.getSiteIndex(x, y, 0)] = 1;
						added[lattice.getSiteIndex(x, y, 0)] = true;
					}
					if (n == 1 && lattice.getSiteType(x, y, lattice.getHeight() - 1) == Site_types[n]) {
						path_distances[lattice.getSiteIndex(x, y, lattice.getHeight() - 1)] = 1;
						added[lattice.getSiteIndex(x, y, lattice.getHeight() - 1)] = true;
					}
				}
			}
			// Initialize the neighbor node vector.
			for (int x = 0; x < lattice.getLength(); x++) {
				for (int y = 0; y < lattice.getWidth(); y++) {
					if (n == 0) {
						z = 1;
					}
					else if (n == 1) {
						z = lattice.getHeight() - 2;
					}
					else {
						continue;
					}
					if (lattice.getSiteType(x, y, z) == Site_types[n]) {
						d = -1;
						coords.setXYZ(x, y, z);
						createNode(temp_node, coords);
						for (int i = 0; i < 26; i++) {
							neighbor_index = temp_node.neighbor_indices[i];
							if (neighbor_index < 0) {
								break;
							}
							if (path_distances[neighbor_index] > 0) {
								d_temp = path_distances[neighbor_index] + sqrt((float)temp_node.neighbor_distances_sq[i]);
								if (d < 0 || d_temp < d) {
									d = d_temp;
								}
							}
						}
						if (d > 0) {
							temp_node.distance_est = d;
							Node_vector[Node_vector_count] = temp_node;
							Node_vector_count++;
							added[temp_node.site_index] = true;
						}
					}
				}
			}
			// The pathfinding algorithm proceeds until there are no nodes left in the neighbor node vector.
			while (!Node_vector_count == 0) {
				// Identify the node with the shortest estimated path distance as the current node.
				d_temp = -1;
				for (int i = 0; i < Node_vector_count; i++) {
					if (d_temp < 0 || Node_vector[i].distance_est < d_temp) {
						current_index = i;
						d_temp = Node_vector[i].distance_est;
					}
				}
				// Insert any unfinalized neighbors of the current node into the neighbor node vector, and check if any already added nodes need to be updated.
				for (int i = 0; i < 26; i++) {
					neighbor_index = Node_vector[current_index].neighbor_indices[i];
					// Check if the target neighbor node is valid.
					if (neighbor_index < 0) {
						break;
					}
					// Check if the target neighbor node has been finalized.
					else if (!(path_distances[neighbor_index] > 0)) {
						// Calculate the estimated path distance to the target neighbor node.
						switch (Node_vector[current_index].neighbor_distances_sq[i]) {
						case 1:
							d = Node_vector[current_index].distance_est + 1;
							break;
						case 2:
							d = Node_vector[current_index].distance_est + sqrt_two;
							break;
						case 3:
							d = Node_vector[current_index].distance_est + sqrt_three;
							break;
						default:
							d = Node_vector[current_index].distance_est;
							break;
						}
						// Check if the target neighbor node has already been added to the Node vector.
						// If not, create the node, update the distance estimate, and add it to the Node vector.
						if (!added[neighbor_index]) {
							coords = lattice.getSiteCoords(neighbor_index);
							createNode(temp_node, coords);
							temp_node.distance_est = d;
							if (Node_vector_count < (int)Node_vector.size()) {
								Node_vector[Node_vector_count] = temp_node;
								Node_vector_count++;
							}
							else {
								Node_vector.push_back(temp_node);
								Node_vector_count++;
							}
							added[neighbor_index] = true;
						}
						// If it has already been added to the node vector, find it, and update the distance estimate only if the new path distance estimate is smaller.
						else {
							// Find the location of the target neighbor node in the node vector
							node_index = -1;
							for (int j = 0; j < Node_vector_count; j++) {
								if (Node_vector[j].site_index == neighbor_index) {
									node_index = j;
									break;
								}
							}
							if (node_index < 0) {
								cout << ID << ": Error! A node designated as added could not be found in the node vector." << endl;
								return false;
							}
							// Update the distance estimate of the neighbor node if a shorter path has been located.
							if (d < Node_vector[node_index].distance_est) {
								Node_vector[node_index].distance_est = d;
							}
						}
					}
				}
				// Finalize the path distance of current node and remove it from the neighbor node vector.
				path_distances[Node_vector[current_index].site_index] = Node_vector[current_index].distance_est;
				Node_vector[current_index] = Node_vector[Node_vector_count - 1];
				Node_vector_count--;
			}
		}
		return true;
	}

	bool Morphology::calculateTortuosity(const char site_type, const bool enable_reduced_memory) {
		bool success;
		bool electrode_num;
		if (site_type == (char)1) {
			electrode_num = false;
		}
		else if (site_type == (char)2) {
			electrode_num = true;
		}
		else {
			cout << ID << ": Error! Tortuosity can only be calculated for site types 1 and 2." << endl;
			return false;
		}
		// The shortest path for each site is stored in the path_distances vector.
		// The path distances are initialized to -1.
		vector<float> path_distances(lattice.getNumSites(), -1.0);
		int site_type_index = getSiteTypeIndex(site_type);
		// Two different path distance calculation implementations are available.
		// The reduced memory implementation uses less memory but takes more calculation time.
		// It is designed to be used when creating large lattices to prevent running out of system memory.
		if (enable_reduced_memory) {
			success = calculatePathDistances_ReducedMemory(path_distances);
		}
		else {
			success = calculatePathDistances(path_distances);
		}
		if (!success) {
			cout << ID << ": Error calculating path distances!" << endl;
			return false;
		}
		// The end-to-end tortuosity describes the tortuosities for the all pathways from the top surface to the bottom surface of the lattice.
		int index;
		Tortuosity_data[site_type_index].assign(lattice.getLength()*lattice.getWidth(), -1);
		for (int x = 0; x < lattice.getLength(); x++) {
			for (int y = 0; y < lattice.getWidth(); y++) {
				index = lattice.getWidth()*x + y;
				if (!electrode_num) {
					if (lattice.getSiteType(x, y, lattice.getHeight() - 1) == site_type && path_distances[lattice.getSiteIndex(x, y, lattice.getHeight() - 1)] > 0) {
						Tortuosity_data[site_type_index][index] = (double)path_distances[lattice.getSiteIndex(x, y, lattice.getHeight() - 1)] / (double)lattice.getHeight();
					}
				}
				else {
					if (lattice.getSiteType(x, y, 0) == site_type && path_distances[lattice.getSiteIndex(x, y, 0)] > 0) {
						Tortuosity_data[site_type_index][index] = (double)path_distances[lattice.getSiteIndex(x, y, 0)] / (double)lattice.getHeight();
					}
				}
			}
		}
		// Any sites which are not connected their respective surface, will have a zero path distance and are identified as part of island domains.
		// Calculate island volume fraction
		Island_volume.assign((int)Site_types.size(), 0);
		for (int x = 0; x < lattice.getLength(); x++) {
			for (int y = 0; y < lattice.getWidth(); y++) {
				for (int z = 0; z < lattice.getHeight(); z++) {
					if (lattice.getSiteType(x, y, z) == Site_types[0] && path_distances[lattice.getSiteIndex(x, y, z)] < 1) {
						Island_volume[0]++;
					}
					if (lattice.getSiteType(x, y, z) == Site_types[1] && path_distances[lattice.getSiteIndex(x, y, z)] < 1) {
						Island_volume[1]++;
					}
				}
			}
		}
		return true;
	}

	//void Morphology::enableThirdNeighborInteraction() {
	//	Enable_third_neighbor_interaction = true;
	//}

	void Morphology::createBilayerMorphology() {
		addSiteType((char)1);
		addSiteType((char)2);
		// Clear existing Site_type_counts data
		for (int n = 0; n < (int)Site_types.size(); n++) {
			Site_type_counts[n] = 0;
		}
		for (int x = 0; x < lattice.getLength(); x++) {
			for (int y = 0; y < lattice.getWidth(); y++) {
				for (int z = 0; z < lattice.getHeight(); z++) {
					if (z < lattice.getHeight() / 2) {
						lattice.setSiteType(x, y, z, (char)1);
						Site_type_counts[0]++;
					}
					else {
						lattice.setSiteType(x, y, z, (char)2);
						Site_type_counts[1]++;
					}
				}
			}
		}
		// This function calculates the actual mix fraction and updates Mix_fraction vector
		calculateMixFractions();
	}

	void Morphology::createCheckerboardMorphology() {
		addSiteType((char)1);
		addSiteType((char)2);
		// Clear existing Site_type_counts data
		for (int n = 0; n < (int)Site_types.size(); n++) {
			Site_type_counts[n] = 0;
		}
		for (int x = 0; x < lattice.getLength(); x++) {
			for (int y = 0; y < lattice.getWidth(); y++) {
				for (int z = 0; z < lattice.getHeight(); z++) {
					if ((x + y + z) % 2 == 0) {
						lattice.setSiteType(x, y, z, (char)1);
						Site_type_counts[0]++;
					}
					else {
						lattice.setSiteType(x, y, z, (char)2);
						Site_type_counts[1]++;
					}
				}
			}
		}
		// This function calculates the actual mix fraction and updates Mix_fraction vector
		calculateMixFractions();
	}

	void Morphology::createNode(Node& node, const Coords& coords) {
		Coords coords_dest;
		for (int i = 0; i < 26; i++) {
			node.neighbor_indices[i] = -1;
			node.neighbor_distances_sq[i] = 0;
		}
		node.site_index = lattice.getSiteIndex(coords);
		int neighbor_count = 0;
		for (int i = -1; i <= 1; i++) {
			for (int j = -1; j <= 1; j++) {
				for (int k = -1; k <= 1; k++) {
					if (!lattice.checkMoveValidity(coords, i, j, k)) {
						continue;
					}
					if (coords.z + k < 0 || coords.z + k >= lattice.getHeight()) {
						continue;
					}
					lattice.calculateDestinationCoords(coords, i, j, k, coords_dest);
					if (lattice.getSiteType(coords) == lattice.getSiteType(coords_dest)) {
						node.neighbor_indices[neighbor_count] = lattice.getSiteIndex(coords_dest);
						node.neighbor_distances_sq[neighbor_count] = (char)(i*i + j * j + k * k);
						neighbor_count++;
					}
				}
			}
		}
	}

	void Morphology::createRandomMorphology(const vector<double>& mix_fractions) {
		for (int n = 0; n < (int)mix_fractions.size(); n++) {
			addSiteType((char)(n + 1));
		}
		double sum = 0;
		for (int n = 0; n < (int)Site_types.size(); n++) {
			if (mix_fractions[n] < 0) {
				cout << ID << ": Error creating random morphology: All mix fractions must be greater than or equal to zero." << endl;
				throw invalid_argument("Error creating random morphology: All mix fractions must be greater than or equal to zero.");
			}
			sum += mix_fractions[n];
		}
		if (fabs(sum - 1.0) > 1e-6) {
			cout << ID << ": Error creating random morphology: Sum of all mix fractions must be equal to one." << endl;
			throw invalid_argument("Error creating random morphology: Sum of all mix fractions must be equal to one.");
		}
		vector<char> type_entries;
		// Calculate the number of sites of each type based on the mix fractions
		for (int n = 0; n < (int)Site_types.size() - 1; n++) {
			Site_type_counts[n] = round_int(mix_fractions[n] * lattice.getNumSites());
			type_entries.insert(type_entries.end(), Site_type_counts[n], Site_types[n]);
		}
		// Whatever sites are left unaccounted for are assigned to the final site type so that there are the correct number of total counts.
		Site_type_counts.back() = lattice.getNumSites() - accumulate(Site_type_counts.begin(), Site_type_counts.end() - 1, 0);
		type_entries.insert(type_entries.end(), Site_type_counts.back(), Site_types.back());
		// Shuffle the site types
		shuffle(type_entries.begin(), type_entries.end(), gen);
		Coords coords;
		for (int x = 0; x < lattice.getLength(); x++) {
			for (int y = 0; y < lattice.getWidth(); y++) {
				for (int z = 0; z < lattice.getHeight(); z++) {
					coords.setXYZ(x, y, z);
					lattice.setSiteType(x, y, z, type_entries[lattice.getSiteIndex(coords)]);
				}
			}
		}
		// This function calculates the actual mix fraction and updates Mix_fraction
		calculateMixFractions();
	}

	void Morphology::executeIsingSwapping(const int num_MCsteps, const double interaction_energy1, const double interaction_energy2, const bool enable_growth_pref, const int growth_direction, const double additional_interaction) {
		int loop_count = 0;
		// N counts the number of MC steps that have been executed
		int N = 0;
		long int main_site_index;
		long int neighbor_site_index;
		char temp;
		double energy_delta, probability;
		Coords main_site_coords;
		std::array<long int, 6> neighbors;
		initializeNeighborInfo();
		// Begin site swapping
		int m = 1;
		while (N < num_MCsteps) {
			// Randomly choose a site in the lattice
			main_site_coords = lattice.generateRandomCoords();
			main_site_index = lattice.getSiteIndex(main_site_coords);
			// If site is not an interfacial site, start over again
			// If total number of first-nearest neighbors = number of first-nearest neighbors of the same type, then the site is not at an interface
			if (Neighbor_info[main_site_index].total1 == Neighbor_counts[main_site_index].sum1) {
				continue;
			}
			// Randomly choose a nearest neighbor site that has a different type
			// Copy all valid indices corresponding to sites with a different type
			auto it = copy_if(Neighbor_info[main_site_index].first_indices.begin(), Neighbor_info[main_site_index].first_indices.end(), neighbors.begin(), [this, main_site_index](long int i) {
				if (i >= 0) {
					return lattice.getSiteType(i) != lattice.getSiteType(main_site_index);
				}
				else {
					return false;
				}
			});
			// Select random dissimilar neighbor site
			uniform_int_distribution<int> dist(0, (int)distance(neighbors.begin(), it) - 1);
			auto selected_it = neighbors.begin();
			advance(selected_it, dist(gen));
			neighbor_site_index = *selected_it;
			// Calculate energy change and swapping probability
			energy_delta = calculateEnergyChangeSimple(main_site_index, neighbor_site_index, interaction_energy1, interaction_energy2);
			if (enable_growth_pref) {
				energy_delta += calculateAdditionalEnergyChange(main_site_index, neighbor_site_index, growth_direction, additional_interaction);
			}
			double E_term = exp(-energy_delta);
			probability = E_term / (1.0 + E_term);
			if (rand01() <= probability) {
				// Swap Sites
				temp = lattice.getSiteType(main_site_index);
				lattice.setSiteType(main_site_index, lattice.getSiteType(neighbor_site_index));
				lattice.setSiteType(neighbor_site_index, temp);
				// Update neighbor counts
				updateNeighborCounts(main_site_index, neighbor_site_index);
			}
			loop_count++;
			// One MC step has been completed when loop_count is equal to the number of sites in the lattice
			if (loop_count == lattice.getNumSites()) {
				N++;
				loop_count = 0;
			}
			if (N == 100 * m) {
				cout << ID << ": " << N << " MC steps completed." << endl;
				m++;
			}
		}
		vector<NeighborCounts>().swap(Neighbor_counts);
		vector<NeighborInfo>().swap(Neighbor_info);
	}

	void Morphology::executeMixing(const double interfacial_width, const double interfacial_conc) {
		vector<int> sites_maj;
		vector<int> sites_min;
		int site_maj;
		int site_min;
		int target;
		int site_count = 0;
		char majority_type;
		char minority_type;
		double minority_conc;
		Coords coords;
		// Based on the interfacial concentration, the majority and minority types are defined
		if (interfacial_conc <= 0.5) {
			majority_type = (char)2;
			minority_type = (char)1;
			minority_conc = interfacial_conc;
		}
		else {
			majority_type = (char)1;
			minority_type = (char)2;
			minority_conc = 1 - interfacial_conc;
		}
		// The minority type sites are mixed into the majority type sites at the interface.
		// First the majority site reservoir is determined by finding all majority type sites within (1-minority_conc)*width distance from the interface and adding them to a list
		for (int x = 0; x < lattice.getLength(); x++) {
			for (int y = 0; y < lattice.getWidth(); y++) {
				for (int z = 0; z < lattice.getHeight(); z++) {
					coords.setXYZ(x, y, z);
					if (lattice.getSiteType(x, y, z) == majority_type) {
						if (isNearInterface(coords, (1 - minority_conc)*interfacial_width)) {
							sites_maj.push_back(lattice.getSiteIndex(x, y, z));
							site_count++;
						}
					}
				}
			}
		}
		// Then the minority site reservoir that will be used to be mix into the majority reservoir to yield the desired final interfacial concentration is determined
		// More minority sites than the number of swaps must be included in the minority reservoir so that the desired interfacial concentration is also reached on the minority side of the interface.
		target = (int)(site_count*minority_conc / (1 - minority_conc));
		int N = 0;
		double range = 1;
		// The minority type sites adjacent to the interface are identified, counted, and added to a list.
		// If this does not yield enough minority sites for swapping, then the range is slowly incremented and minority sites farther from the interface are included until there are enough sites.
		while (N < target) {
			sites_min.clear();
			N = 0;
			for (int x = 0; x < lattice.getLength(); x++) {
				for (int y = 0; y < lattice.getWidth(); y++) {
					for (int z = 0; z < lattice.getHeight(); z++) {
						coords.setXYZ(x, y, z);
						if (lattice.getSiteType(x, y, z) == minority_type) {
							if (isNearInterface(coords, range)) {
								sites_min.push_back(lattice.getSiteIndex(x, y, z));
								N++;
							}
						}
					}
				}
			}
			range += 0.1;
		}
		// Sites are randomly chosen from the minority and majority reservoirs and swapped until the desired interfacial concentration is reached.
		for (int i = 0; i < site_count*minority_conc; i++) {
			uniform_int_distribution<int> dist_maj(0, (int)sites_maj.size() - 1);
			auto rand_maj = bind(dist_maj, ref(gen));
			uniform_int_distribution<int> dist_min(0, (int)sites_min.size() - 1);
			auto rand_min = bind(dist_min, ref(gen));
			site_maj = rand_maj();
			site_min = rand_min();
			lattice.setSiteType(sites_maj[site_maj], minority_type);
			lattice.setSiteType(sites_min[site_min], majority_type);
			// Both site types are removed from the lists once they are swapped to prevent unswapping.
			sites_maj.erase(sites_maj.begin() + site_maj);
			sites_min.erase(sites_min.begin() + site_min);
		}
	}

	void Morphology::executeSmoothing(const double smoothing_threshold, const int rescale_factor) {
		double roughness_factor;
		Coords coords, coords_dest;
		int radius = (rescale_factor <= 2) ? 1 : (int)ceil((double)(rescale_factor + 1) / 2);
		int cutoff_squared = (rescale_factor <= 2) ? 2 : (int)floor(intpow((rescale_factor + 1.0) / 2.0, 2));
		// The boolean vector consider_smoothing keeps track of whether each site is near the interface and should be considered for smoothing.
		// Sites in the interior of the domains or at very smooth interfaces do not need to be continually reconsidered for smoothing.
		vector<bool> consider_smoothing;
		consider_smoothing.assign(lattice.getNumSites(), true);
		int site_count = 1;
		while (site_count > 0) {
			site_count = 0;
			for (int x = 0; x < lattice.getLength(); x++) {
				for (int y = 0; y < lattice.getWidth(); y++) {
					for (int z = 0; z < lattice.getHeight(); z++) {
						if (!consider_smoothing[lattice.getSiteIndex(x, y, z)]) {
							continue;
						}
						coords.setXYZ(x, y, z);
						// Calculate the roughness factor of the target site.
						roughness_factor = calculateDissimilarFraction(coords, rescale_factor);
						// Swap the site's type if the roughness_factor is greater than the smoothing_threshold.
						if (roughness_factor > smoothing_threshold) {
							if (lattice.getSiteType(x, y, z) == (char)1) {
								lattice.setSiteType(x, y, z, (char)2);
							}
							else if (lattice.getSiteType(x, y, z) == (char)2) {
								lattice.setSiteType(x, y, z, (char)1);
							}
							site_count++;
							// When a site swaps types, all surrounding sites must be reconsidered for smoothing.
							for (int i = -radius; i <= radius; i++) {
								for (int j = -radius; j <= radius; j++) {
									for (int k = -radius; k <= radius; k++) {
										if (i*i + j * j + k * k > cutoff_squared) {
											continue;
										}
										if (!lattice.checkMoveValidity(coords, i, j, k)) {
											continue;
										}
										lattice.calculateDestinationCoords(coords, i, j, k, coords_dest);
										consider_smoothing[lattice.getSiteIndex(coords_dest)] = true;
									}
								}
							}
						}
						// Sites with a low roughness_factor are not swapped and are removed from reconsideration.
						else {
							consider_smoothing[lattice.getSiteIndex(x, y, z)] = false;
						}
					}
				}
			}
		}
		// The smoothing process can change the mix fraction, so the final mix fraction is recalculated and the Mix_fraction property is updated.
		calculateMixFractions();
	}

	vector<double> Morphology::getCorrelationData(const char site_type) const {
		if (Correlation_data[getSiteTypeIndex(site_type)][0] == 0) {
			cout << ID << ": Error getting correlation data: Correlation data has not been calculated." << endl;
		}
		return Correlation_data[getSiteTypeIndex(site_type)];
	}

	vector<double> Morphology::getDepthCompositionData(const char site_type) const {
		return Depth_composition_data[getSiteTypeIndex(site_type)];
	}

	vector<double> Morphology::getDepthDomainSizeData(const char site_type) const {
		return Depth_domain_size_data[getSiteTypeIndex(site_type)];
	}

	vector<double> Morphology::getDepthIVData() const {
		return Depth_iv_data;
	}

	double Morphology::getDomainAnisotropy(const char site_type) const {
		return Domain_anisotropies[getSiteTypeIndex(site_type)];
	}

	double Morphology::getDomainSize(char site_type) const {
		return Domain_sizes[getSiteTypeIndex(site_type)];
	}

	int Morphology::getHeight() const {
		return lattice.getHeight();
	}

	int Morphology::getID() const {
		return ID;
	}

	vector < pair<double, int>> Morphology::getInterfacialDistanceHistogram(char site_type) const {
		return InterfacialHistogram_data[getSiteTypeIndex(site_type)];
	}

	double Morphology::getIslandVolumeFraction(char site_type) const {
		return (double)Island_volume[getSiteTypeIndex(site_type)] / (double)lattice.getNumSites();
	}

	int Morphology::getLength() const {
		return lattice.getLength();
	}

	double Morphology::getMixFraction(const char site_type) const {
		return Mix_fractions[getSiteTypeIndex(site_type)];
	}

	void Morphology::getSiteSampling(vector<long int>& site_indices, const char site_type, const int N_sites_max) {
		vector<long int> all_sites(Site_type_counts[getSiteTypeIndex(site_type)], 0);
		int m = 0;
		for (long n = 0; n < lattice.getNumSites(); n++) {
			if (lattice.getSiteType(n) == site_type && m < (int)all_sites.size()) {
				all_sites[m] = n;
				m++;
			}
		}
		shuffle(all_sites.begin(), all_sites.end(), gen);
		if (N_sites_max > (int)all_sites.size()) {
			site_indices.assign(all_sites.begin(), all_sites.end());
		}
		else {
			site_indices.assign(all_sites.begin(), all_sites.begin() + N_sites_max);
		}
	}

	void Morphology::getSiteSamplingZ(vector<long int>& site_indices, const char site_type, const int N_sites_max, const int z) {
		vector<long int> all_sites;
		for (int x = 0; x < lattice.getLength(); x++) {
			for (int y = 0; y < lattice.getWidth(); y++) {
				if (lattice.getSiteType(x, y, z) == site_type) {
					all_sites.push_back(lattice.getSiteIndex(x, y, z));
				}
			}
		}
		shuffle(all_sites.begin(), all_sites.end(), gen);
		if (N_sites_max > (int)all_sites.size()) {
			site_indices.assign(all_sites.begin(), all_sites.end());
		}
		else {
			site_indices.assign(all_sites.begin(), all_sites.begin() + N_sites_max);
		}
	}

	int Morphology::getSiteTypeIndex(const char site_type) const {
		for (int i = 0; i < (int)Site_types.size(); i++) {
			if (site_type == Site_types[i]) {
				return i;
			}
		}
		cout << ID << ": Error! Site type " << (int)site_type << " was not found in the Site_types vector." << endl;
		throw invalid_argument("Error! Input site type was not found in the Site_types vector.");
	}

	vector<double> Morphology::getTortuosityData(char site_type) const {
		vector<double> output_data;
		for (int i = 0; i < (int)Tortuosity_data[getSiteTypeIndex(site_type)].size(); i++) {
			if (Tortuosity_data[getSiteTypeIndex(site_type)][i] > 0) {
				output_data.push_back(Tortuosity_data[getSiteTypeIndex(site_type)][i]);
			}
		}
		return output_data;
	}

	double Morphology::getUnitSize() const {
		return lattice.getUnitSize();
	}

	int Morphology::getWidth() const {
		return lattice.getWidth();
	}

	vector<Morphology> Morphology::importTomogramMorphologyFile() {
		vector<Morphology> morphologies;
		if (!Params.Enable_import_tomogram) {
			cout << ID << ": Error! Attempting to import tomogram data when tomogram import is not enabled in the parameters." << endl;
			throw runtime_error("Error! Attempting to import tomogram data when tomogram import is not enabled in the parameters.");
		}
		string metadata_filename = Params.Tomogram_name + ".xml";
		string data_filename = Params.Tomogram_name + ".raw";
		// Parse XML metadata file
		cout << ID << ": Loading and Parsing the XML metadata file..." << endl;
		XMLDocument xml_doc;
		string data_format;
		FILE* xml_file_ptr;
		xml_file_ptr = fopen(metadata_filename.c_str(), "rb");
		if (xml_file_ptr == NULL) {
			cout << ID << ": Error! XML metadata file not found." << endl;
			throw runtime_error("Error! XML metadata file not found.");
		}
		xml_doc.LoadFile(xml_file_ptr);
		if (xml_doc.Error()) {
			xml_doc.PrintError();
			cout << ID << ": Error loading XML metadata file." << endl;
			throw runtime_error("Error loading XML metadata file.");
		}
		// Analyze XML info file based on metadata format version
		Version required_version("1.0.0");
		string schema_version_str = xml_doc.FirstChildElement("tomogram_metadata")->Attribute("schema_version");
		Version schema_version(schema_version_str);
		// Initialize lattice params based on xml data
		Lattice::Lattice_Params lattice_params;
		if (schema_version == required_version) {
			lattice_params.Length = atoi(xml_doc.FirstChildElement("tomogram_metadata")->FirstChildElement("data_info")->FirstChildElement("length")->GetText());
			lattice_params.Width = atoi(xml_doc.FirstChildElement("tomogram_metadata")->FirstChildElement("data_info")->FirstChildElement("width")->GetText());
			lattice_params.Height = atoi(xml_doc.FirstChildElement("tomogram_metadata")->FirstChildElement("data_info")->FirstChildElement("height")->GetText());
			lattice_params.Unit_size = atof(xml_doc.FirstChildElement("tomogram_metadata")->FirstChildElement("data_info")->FirstChildElement("pixel_size")->FirstChildElement("value")->GetText());
			// Save additional xml data needed
			data_format = xml_doc.FirstChildElement("tomogram_metadata")->FirstChildElement("data_info")->FirstChildElement("data_format")->GetText();
			if (data_format.compare("8 bit") != 0 && data_format.compare("16 bit") != 0) {
				cout << "Error! The xml metadata file does not specify a valid data format. Only 8 bit and 16 bit formats are supported." << endl;
				throw runtime_error("Error! The xml metadata file does not specify a valid data format. Only 8 bit and 16 bit formats are supported.");
			}
			// Initialize Morphology site types
			int site_type_index = 0;
			XMLElement* element_ptr = xml_doc.FirstChildElement("tomogram_metadata")->FirstChildElement("sample_info")->FirstChildElement("composition_info")->FirstChildElement("chemical_component");
			while (element_ptr != 0) {
				addSiteType((char)(site_type_index + 1));
				Mix_fractions[site_type_index] = atof(element_ptr->FirstChildElement("vol_frac")->GetText());
				element_ptr = element_ptr->NextSiblingElement("chemical_component");
				site_type_index++;
			}
		}
		else {
			cout << ID << ": Error! XML metadata schema version not supported. Only v1.0.0 is supported." << endl;
			throw runtime_error("Error! XML metadata schema version not supported. Only v1.0.0 is supported.");
		}
		fclose(xml_file_ptr);
		double Mix_fraction_import = Mix_fractions[0];
		// Construct initial lattice based on XML metadata
		Lattice lattice_i;
		lattice_i.init(lattice_params);
		// Open and read data file
		cout << ID << ": Loading RAW data file..." << endl;
		ifstream data_file(data_filename, ifstream::in | ifstream::binary);
		if (!data_file.is_open()) {
			cout << ID << ": Error! Tomogram binary RAW file " << data_filename << " could not be opened." << endl;
			throw runtime_error("Error! Tomogram binary RAW file could not be opened.");
		}
		data_file.seekg(0, data_file.end);
		int N_bytes = (int)data_file.tellg();
		data_file.seekg(0, data_file.beg);
		vector<float> data_vec;
		if (data_format.compare("8 bit") == 0) {
			unsigned char* data_block = new unsigned char[N_bytes];
			data_file.read((char*)data_block, N_bytes);
			data_vec.resize(N_bytes);
			for (int i = 0; i < (int)data_vec.size(); i++) {
				data_vec[i] = (float)data_block[i];
			}
			delete[] data_block;
		}
		else if (data_format.compare("16 bit") == 0) {
			char16_t* data_block = new char16_t[N_bytes / 2];
			data_file.read((char*)data_block, N_bytes);
			data_vec.resize(N_bytes / 2);
			for (int i = 0; i < (int)data_vec.size(); i++) {
				data_vec[i] = (float)data_block[i];
			}
			delete[] data_block;
		}
		data_file.close();
		// Check amount of data read from the RAW file
		if ((int)data_vec.size() != lattice_i.getNumSites()) {
			cout << "Error! The imported tomogram RAW file does not contain the correct number of sites." << endl;
			cout << "The initial lattice has " << lattice_i.getNumSites() << " sites but the data file has " << data_vec.size() << " entries." << endl;
			throw runtime_error("Error! The imported tomogram RAW file does not contain the correct number of sites.");
		}
		// Rotate image data to standard Ising_OPV reference frame
		int index = 0;
		vector<float> data_vec_new(data_vec.size());
		for (int k = 0; k < lattice_i.getHeight(); k++) {
			for (int j = 0; j < lattice_i.getWidth(); j++) {
				for (int i = 0; i < lattice_i.getLength(); i++) {
					data_vec_new[lattice_i.getSiteIndex(i, j, k)] = data_vec[index];
					index++;
				}
			}
		}
		vector<float>().swap(data_vec);
		// Construct lattice with desired unit size
		cout << ID << ": Interpolating data to construct final lattice..." << endl;
		// Determine final lattice dimensions based on desired unit size
		lattice_params.Length = (int)floor(lattice_params.Length*(lattice_params.Unit_size / Params.Desired_unit_size));
		lattice_params.Width = (int)floor(lattice_params.Width*(lattice_params.Unit_size / Params.Desired_unit_size));
		lattice_params.Height = (int)floor(lattice_params.Height*(lattice_params.Unit_size / Params.Desired_unit_size));
		lattice_params.Unit_size = Params.Desired_unit_size;
		lattice.init(lattice_params);
		vector<float> data_vec_final(lattice.getNumSites());
		for (int i = 0; i < lattice.getLength(); i++) {
			for (int j = 0; j < lattice.getWidth(); j++) {
				for (int k = 0; k < lattice.getHeight(); k++) {
					double x = i * lattice.getUnitSize();
					double y = j * lattice.getUnitSize();
					double z = k * lattice.getUnitSize();
					static vector<float> weights(8, 0.0);
					static vector<float> vals(8, 0.0);
					int x1 = (int)floor(x / lattice_i.getUnitSize());
					int x2 = (int)ceil(x / lattice_i.getUnitSize());
					int y1 = (int)floor(y / lattice_i.getUnitSize());
					int y2 = (int)ceil(y / lattice_i.getUnitSize());
					int z1 = (int)floor(z / lattice_i.getUnitSize());
					int z2 = (int)ceil(z / lattice_i.getUnitSize());
					if ((x - x1 * lattice_i.getUnitSize()) < 1e-6 && (y - y1 * lattice_i.getUnitSize()) < 1e-6 && (z - z1 * lattice_i.getUnitSize()) < 1e-6) {
						data_vec_final[lattice.getSiteIndex(i, j, k)] = data_vec_new[lattice_i.getSiteIndex(x1, y1, z1)];
						continue;
					}
					if (x2 >= lattice_i.getLength()) {
						x2 = lattice_i.getLength() - 1;
					}
					if (y2 >= lattice_i.getWidth()) {
						y2 = lattice_i.getWidth() - 1;
					}
					if (z2 >= lattice_i.getHeight()) {
						z2 = lattice_i.getHeight() - 1;
					}
					vals[0] = data_vec_new[lattice_i.getSiteIndex(x1, y1, z1)];
					vals[1] = data_vec_new[lattice_i.getSiteIndex(x1, y2, z1)];
					vals[2] = data_vec_new[lattice_i.getSiteIndex(x2, y1, z1)];
					vals[3] = data_vec_new[lattice_i.getSiteIndex(x2, y2, z1)];
					vals[4] = data_vec_new[lattice_i.getSiteIndex(x1, y1, z2)];
					vals[5] = data_vec_new[lattice_i.getSiteIndex(x1, y2, z2)];
					vals[6] = data_vec_new[lattice_i.getSiteIndex(x2, y1, z2)];
					vals[7] = data_vec_new[lattice_i.getSiteIndex(x2, y2, z2)];
					weights[0] = (float)(1.0 / (intpow(x - x1 * lattice_i.getUnitSize(), 2) + intpow(y - y1 * lattice_i.getUnitSize(), 2) + intpow(z - z1 * lattice_i.getUnitSize(), 2)));
					weights[1] = (float)(1.0 / (intpow(x - x1 * lattice_i.getUnitSize(), 2) + intpow(y - y2 * lattice_i.getUnitSize(), 2) + intpow(z - z1 * lattice_i.getUnitSize(), 2)));
					weights[2] = (float)(1.0 / (intpow(x - x2 * lattice_i.getUnitSize(), 2) + intpow(y - y1 * lattice_i.getUnitSize(), 2) + intpow(z - z1 * lattice_i.getUnitSize(), 2)));
					weights[3] = (float)(1.0 / (intpow(x - x2 * lattice_i.getUnitSize(), 2) + intpow(y - y2 * lattice_i.getUnitSize(), 2) + intpow(z - z1 * lattice_i.getUnitSize(), 2)));
					weights[4] = (float)(1.0 / (intpow(x - x1 * lattice_i.getUnitSize(), 2) + intpow(y - y1 * lattice_i.getUnitSize(), 2) + intpow(z - z2 * lattice_i.getUnitSize(), 2)));
					weights[5] = (float)(1.0 / (intpow(x - x1 * lattice_i.getUnitSize(), 2) + intpow(y - y2 * lattice_i.getUnitSize(), 2) + intpow(z - z2 * lattice_i.getUnitSize(), 2)));
					weights[6] = (float)(1.0 / (intpow(x - x2 * lattice_i.getUnitSize(), 2) + intpow(y - y1 * lattice_i.getUnitSize(), 2) + intpow(z - z2 * lattice_i.getUnitSize(), 2)));
					weights[7] = (float)(1.0 / (intpow(x - x2 * lattice_i.getUnitSize(), 2) + intpow(y - y2 * lattice_i.getUnitSize(), 2) + intpow(z - z2 * lattice_i.getUnitSize(), 2)));
					std::transform(vals.begin(), vals.end(), weights.begin(), vals.begin(), multiplies<float>());
					data_vec_final[lattice.getSiteIndex(i, j, k)] = accumulate(vals.begin(), vals.end(), 0.0f) / accumulate(weights.begin(), weights.end(), 0.0f);
				}
			}
		}
		vector<float>().swap(data_vec_new);
		// Use pixel brightness cutoff method to assign site types, dark sites are type2 and bright sites are type1
		cout << ID << ": Analyzing pixel brightness to assign site types..." << endl;
		// Determine number of each type of site
		int N_type1_total = round_int(Mix_fraction_import * lattice.getNumSites());
		int N_type2_total = lattice.getNumSites() - N_type1_total;
		int N_mixed_total = round_int(Params.Mixed_frac * lattice.getNumSites());
		int N_type1_mixed = round_int(Params.Mixed_conc * N_mixed_total);
		int N_type2_mixed = N_mixed_total - N_type1_mixed;
		int N_type1_pure = N_type1_total - N_type1_mixed;
		int N_type2_pure = N_type2_total - N_type2_mixed;
		// Create vector of indices sorted by the corresponding pixel brightness
		vector<int> indices(data_vec_final.size());
		iota(indices.begin(), indices.end(), 0);
		sort(indices.begin(), indices.end(), [&data_vec_final](const int& a, const int& b) {
			return (data_vec_final[a] < data_vec_final[b]);
		});
		// Assign indices to a site type from pure regions
		vector<int> type1_indices;
		type1_indices.insert(type1_indices.end(), indices.end() - N_type1_pure, indices.end());
		vector<int> type2_indices;
		type2_indices.insert(type2_indices.end(), indices.begin(), indices.begin() + N_type2_pure);
		// Add the remaining indices to a vector of mixed indices
		vector<int> mixed_indices;
		mixed_indices.insert(mixed_indices.end(), indices.begin() + N_type2_pure, indices.end() - N_type1_pure);
		// Shuffle the mixed indices
		shuffle(mixed_indices.begin(), mixed_indices.end(), gen);
		// Assign mixed indices as type1 or type2 
		type1_indices.insert(type1_indices.end(), mixed_indices.begin(), mixed_indices.begin() + N_type1_mixed);
		type2_indices.insert(type2_indices.end(), mixed_indices.begin() + N_type1_mixed, mixed_indices.end());
		// Clear existing Site_type_counts data
		for (int n = 0; n < (int)Site_types.size(); n++) {
			Site_type_counts[n] = 0;
		}
		// Assign sites based on indices vectors
		for (auto& item : type1_indices) {
			lattice.setSiteType(item, (char)1);
			Site_type_counts[getSiteTypeIndex((char)1)]++;
		}
		for (auto& item : type2_indices) {
			lattice.setSiteType(item, (char)2);
			Site_type_counts[getSiteTypeIndex((char)2)]++;
		}
		// Check that all sites were assigned a type
		if (lattice.getNumSites() != accumulate(Site_type_counts.begin(), Site_type_counts.end(), 0)) {
			cout << ID << ": Error importing morphology file. All sites were not assigned to a valid site type." << endl;
			throw runtime_error("Error importing morphology file. All sites were not assigned to a valid site type.");
		}
		// Check the final mix fractions
		calculateMixFractions();
		if (abs(Mix_fractions[0] - Mix_fraction_import) > 0.01) {
			cout << ID << ": Error importing morphology file. Final type1 mix fraction does not match the mix fraction designated in the xml metadata file." << endl;
			throw runtime_error("Error importing morphology file. Final type1 mix fraction does not match the mix fraction designated in the xml metadata file.");
		}
		// Generate morphology set
		cout << ID << ": Creating morphology set from tomogram data." << endl;
		if (Params.N_extracted_segments > 1) {
			int dim = round_int(sqrt(Params.N_extracted_segments));
			int new_length = lattice.getLength() / dim;
			// make dimensions even
			if (new_length % 2 != 0) {
				new_length--;
			}
			int new_width = lattice.getWidth() / dim;
			if (new_width % 2 != 0) {
				new_width--;
			}
			int size = (new_length < new_width) ? new_length : new_width;
			int offset_x = (lattice.getLength() - size * dim) / 2;
			int offset_y = (lattice.getWidth() - size * dim) / 2;
			Lattice sublattice;
			Parameters params_new = Params;
			params_new.Length = size;
			params_new.Width = size;
			params_new.Height = lattice.getHeight();
			params_new.Enable_periodic_z = false;
			for (int x = 0; x < dim; x++) {
				for (int y = 0; y < dim; y++) {
					sublattice = lattice.extractSublattice(x*size + offset_x, size, y*size + offset_y, size, 0, lattice.getHeight());
					morphologies.push_back(Morphology(sublattice, params_new, x * dim + y));
					morphologies[x * dim + y].calculateMixFractions();
				}
			}
		}
		else {
			morphologies.push_back(*this);
		}
		return morphologies;
	}

	bool Morphology::importMorphologyFile(ifstream& infile) {
		Version min_version("4.0.0-beta.1");
		string line;
		bool is_file_compressed;
		Lattice::Lattice_Params lattice_params;
		lattice_params.Unit_size = 1.0;
		// Check status of input filestream
		if (!infile.is_open() || !infile.good()) {
			cout << ID << ": Error importing morphology file. Input filestream is not open or is not a good state." << endl;
			return false;
		}
		// Load file lines into string vector
		vector<string> file_data;
		while (getline(infile, line)) {
			file_data.push_back(line);
		}
		// Analyze file header line
		if (file_data[0].substr(0, 9).compare("Ising_OPV") == 0) {
			// extract version string
			string version_str = file_data[0];
			version_str.erase(0, version_str.find('v') + 1);
			version_str = version_str.substr(0, version_str.find(' '));
			Version file_version(version_str);
			// check if morphology file version is greater than or equal to the minimum version
			if (file_version < min_version) {
				cout << ID << ": Error importing morphology file. Only morphology files generated by Ising_OPV v4.0.0-beta.1 or greater are supported." << endl;
				return false;
			}
			// Check if file is in compressed format or not
			is_file_compressed = (file_data[0].find("uncompressed") == string::npos);
		}
		else {
			cout << ID << ": Error importing morphology file. Incorrect file format. Only morphology files generated by Ising_OPV v4.0.0-beta.1 or greater are supported." << endl;
			return false;
		}
		// Get lattice dimensions
		lattice_params.Length = atoi(file_data[1].c_str());
		lattice_params.Width = atoi(file_data[2].c_str());
		lattice_params.Height = atoi(file_data[3].c_str());
		// Get lattice periodicity options
		lattice_params.Enable_periodic_x = (bool)atoi(file_data[4].c_str());
		lattice_params.Enable_periodic_y = (bool)atoi(file_data[5].c_str());
		lattice_params.Enable_periodic_z = (bool)atoi(file_data[6].c_str());
		// Create the lattice
		lattice.init(lattice_params);
		// Get number of site types
		int num_types = atoi(file_data[7].c_str());
		for (int n = 0; n < num_types; n++) {
			addSiteType((char)(n + 1));
		}
		// Get domain size for each site type
		for (int n = 0; n < num_types; n++) {
			Domain_sizes[n] = atof(file_data[8 + n].c_str());
		}
		// Get mix fraction for each site type
		for (int n = 0; n < num_types; n++) {
			Mix_fractions[n] = atof(file_data[8 + num_types + n].c_str());
		}
		// Clear existing Site_type_counts data
		for (int n = 0; n < (int)Site_types.size(); n++) {
			Site_type_counts[n] = 0;
		}
		if (!is_file_compressed) {
			for (int i = 8 + 2 * num_types; i < (int)file_data.size(); i++) {
				stringstream linestream(file_data[i]);
				string item;
				vector<int> values;
				values.reserve(4);
				while (getline(linestream, item, ',')) {
					values.push_back(atoi(item.c_str()));
				}
				lattice.setSiteType(values[0], values[1], values[2], (char)values[3]);
				Site_type_counts[getSiteTypeIndex((char)values[3])]++;
			}
			// Check that all sites were assigned a type
			if (lattice.getNumSites() != accumulate(Site_type_counts.begin(), Site_type_counts.end(), 0)) {
				cout << ID << ": Error importing morphology file. All sites were not assigned to a valid site type." << endl;
				return false;
			}
		}
		else {
			int site_count = 0;
			int line_count = 8 + 2 * num_types;
			char type = 0;
			for (int x = 0; x < lattice.getLength(); x++) {
				for (int y = 0; y < lattice.getWidth(); y++) {
					for (int z = 0; z < lattice.getHeight(); z++) {
						if (site_count == 0) {
							if (line_count == (int)file_data.size()) {
								cout << ID << ": Error parsing input morphology file. End of file reached before expected." << endl;
								return false;
							}
							type = (char)atoi(file_data[line_count].substr(0, 1).c_str());
							site_count = atoi(file_data[line_count].substr(1).c_str());
							line_count++;
						}
						lattice.setSiteType(x, y, z, type);
						Site_type_counts[getSiteTypeIndex(type)]++;
						site_count--;
					}
				}
			}
		}
		calculateMixFractions();
		return true;
	}

	void Morphology::initializeNeighborInfo() {
		Coords coords, coords_dest;
		char sum1, sum2, sum3;
		char total1, total2, total3;
		int first_neighbor_count, second_neighbor_count, third_neighbor_count;
		char site_type;
		// Initialize neighbor counts (this data is used in the calculateEnergyChangeSimple function)
		NeighborCounts counts;
		Neighbor_counts.assign(lattice.getNumSites(), counts);
		NeighborInfo info;
		Neighbor_info.assign(lattice.getNumSites(), info);
		for (int x = 0; x < lattice.getLength(); x++) {
			for (int y = 0; y < lattice.getWidth(); y++) {
				for (int z = 0; z < lattice.getHeight(); z++) {
					coords.setXYZ(x, y, z);
					sum1 = 0;
					sum2 = 0;
					sum3 = 0;
					// There are in total 6 first-nearest, 12 second-nearest, and 8 third-nearest neighbors
					total1 = 6;
					total2 = 12;
					total3 = 8;
					// Keep track of which neighbors have been calculated
					first_neighbor_count = 0;
					second_neighbor_count = 0;
					third_neighbor_count = 0;
					// Calculate similar neighbors around x,y,z
					site_type = lattice.getSiteType(x, y, z);
					for (int i = -1; i <= 1; i++) {
						for (int j = -1; j <= 1; j++) {
							for (int k = -1; k <= 1; k++) {
								if (i == 0 && j == 0 && k == 0) {
									continue;
								}
								if (!lattice.isZPeriodic()) {
									if (z + k >= lattice.getHeight() || z + k < 0) { // Check for z boundary
										// Total site counts must be reduced if next to a hard boundary
										switch (i*i + j * j + k * k) {
										case 1:
											total1--;
											Neighbor_info[lattice.getSiteIndex(x, y, z)].first_indices[first_neighbor_count] = -1;
											first_neighbor_count++;
											break;
										case 2:
											total2--;
											Neighbor_info[lattice.getSiteIndex(x, y, z)].second_indices[second_neighbor_count] = -1;
											second_neighbor_count++;
											break;
										case 3:
											total3--;
											Neighbor_info[lattice.getSiteIndex(x, y, z)].third_indices[third_neighbor_count] = -1;
											third_neighbor_count++;
											break;
										default:
											break;
										}
										continue;
									}
								}
								lattice.calculateDestinationCoords(coords, i, j, k, coords_dest);
								// Count the number of similar neighbors
								if (lattice.getSiteType(coords_dest) == site_type) {
									switch (i*i + j * j + k * k) {
									case 1:
										sum1++;
										break;
									case 2:
										sum2++;
										break;
									case 3:
										sum3++;
										break;
									default:
										break;
									}
								}
								// Determine neighbor site indices
								switch (i*i + j * j + k * k) {
								case 1:
									Neighbor_info[lattice.getSiteIndex(x, y, z)].first_indices[first_neighbor_count] = lattice.getSiteIndex(coords_dest);
									first_neighbor_count++;
									break;
								case 2:
									Neighbor_info[lattice.getSiteIndex(x, y, z)].second_indices[second_neighbor_count] = lattice.getSiteIndex(coords_dest);
									second_neighbor_count++;
									break;
								case 3:
									Neighbor_info[lattice.getSiteIndex(x, y, z)].third_indices[third_neighbor_count] = lattice.getSiteIndex(coords_dest);
									third_neighbor_count++;
									break;
								default:
									break;
								}
							}
						}
					}
					Neighbor_counts[lattice.getSiteIndex(x, y, z)].sum1 = sum1;
					Neighbor_counts[lattice.getSiteIndex(x, y, z)].sum2 = sum2;
					Neighbor_counts[lattice.getSiteIndex(x, y, z)].sum3 = sum3;
					Neighbor_info[lattice.getSiteIndex(x, y, z)].total1 = total1;
					Neighbor_info[lattice.getSiteIndex(x, y, z)].total2 = total2;
					Neighbor_info[lattice.getSiteIndex(x, y, z)].total3 = total3;
					if (!(Neighbor_counts[lattice.getSiteIndex(x, y, z)] == calculateNeighborCounts(coords))) {
						cout << "Error initializing neighbor counts!" << endl;
					}
				}
			}
		}
	}

	bool Morphology::isNearInterface(const Coords& coords, const double distance) const {
		int range = (int)ceil(distance);
		double distance_sq = distance * distance;
		Coords coords_dest;
		for (int i = -range; i <= range; i++) {
			for (int j = -range; j <= range; j++) {
				for (int k = -range; k <= range; k++) {
					if (abs(i) + abs(j) + abs(k) > range) {
						continue;
					}
					else if ((double)(i*i + j * j + k * k) > distance_sq) {
						continue;
					}
					if (!lattice.checkMoveValidity(coords, i, j, k)) {
						continue;
					}
					lattice.calculateDestinationCoords(coords, i, j, k, coords_dest);
					if (lattice.getSiteType(coords) != lattice.getSiteType(coords_dest)) {
						return true;
					}
				}
			}
		}
		return false;
	}

	void Morphology::outputCompositionMaps(ofstream& outfile) const {
		vector<int> counts(Site_types.size(), 0);
		outfile << "X-Position,Y-Position";
		for (int n = 0; n < (int)Site_types.size(); n++) {
			outfile << ",Composition" << (int)Site_types[n];
		}
		outfile << endl;
		for (int x = 0; x < lattice.getLength(); x++) {
			for (int y = 0; y < lattice.getWidth(); y++) {
				counts.assign(Site_types.size(), 0);
				for (int z = 0; z < lattice.getHeight(); z++) {
					counts[getSiteTypeIndex(lattice.getSiteType(x, y, z))]++;
				}
				outfile << x << "," << y;
				for (int n = 0; n < (int)Site_types.size(); n++) {
					outfile << "," << (double)counts[n] / (double)lattice.getHeight();
				}
				outfile << endl;
			}
		}
	}

	void Morphology::outputCorrelationData(ofstream& outfile) const {
		// Output Calculation Results
		int max_correlation_size = (int)Correlation_data[0].size();
		for (int n = 1; n < (int)Site_types.size(); n++) {
			if ((int)Correlation_data[n].size() > max_correlation_size) {
				max_correlation_size = (int)Correlation_data[n].size();
			}
		}
		outfile << "Distance (nm)";
		for (int n = 0; n < (int)Site_types.size(); n++) {
			outfile << ",Correlation" << (int)Site_types[n];
		}
		outfile << endl;
		for (int i = 0; i < max_correlation_size; i++) {
			if (i < (int)Correlation_data[0].size()) {
				outfile << lattice.getUnitSize()*0.5*(double)i << "," << Correlation_data[0][i];
			}
			else {
				outfile << lattice.getUnitSize()*0.5*(double)i << "," << NAN;
			}
			for (int n = 1; n < (int)Site_types.size(); n++) {
				if (i < (int)Correlation_data[n].size()) {
					outfile << "," << Correlation_data[n][i];
				}
				else {
					outfile << "," << NAN;
				}
			}
			outfile << endl;
		}
	}

	void Morphology::outputDepthDependentData(ofstream& outfile) const {
		outfile << "Z-Position";
		for (int n = 0; n < (int)Site_types.size(); n++) {
			outfile << ",Type" << (int)Site_types[n] << "_composition";
		}
		for (int n = 0; n < (int)Site_types.size(); n++) {
			outfile << ",Type" << (int)Site_types[n] << "_domain_size";
		}
		outfile << ",IV_fraction";
		outfile << endl;
		for (int z = 0; z < lattice.getHeight(); z++) {
			outfile << z;
			for (int n = 0; n < (int)Site_types.size(); n++) {
				outfile << "," << Depth_composition_data[n][z];
			}
			for (int n = 0; n < (int)Site_types.size(); n++) {
				outfile << "," << Depth_domain_size_data[n][z];
			}
			outfile << "," << Depth_iv_data[z];
			outfile << endl;
		}
	}

	void Morphology::outputMorphologyCrossSection(ofstream& outfile) const {
		outfile << "X-Position,Y-Position,Z-Position,Site_type" << endl;
		int x = lattice.getLength() / 2;
		//for (int x = 0; x < lattice.getLength(); x++) {
		for (int y = 0; y < lattice.getWidth(); y++) {
			//int z = lattice.getHeight() / 2;
			for (int z = 0; z < lattice.getHeight(); z++) {
				outfile << x << "," << y << "," << z << "," << (int)lattice.getSiteType(x, y, z) << endl;
			}
		}
		//}
	}

	void Morphology::outputMorphologyFile(ofstream& outfile, bool enable_export_compressed) const {
		if (enable_export_compressed) {
			outfile << "Ising_OPV v" << Current_version.getVersionStr() << " - compressed format" << endl;
		}
		else {
			outfile << "Ising_OPV v" << Current_version.getVersionStr() << " - uncompressed format" << endl;
		}
		outfile << lattice.getLength() << endl;
		outfile << lattice.getWidth() << endl;
		outfile << lattice.getHeight() << endl;
		outfile << lattice.isXPeriodic() << endl;
		outfile << lattice.isYPeriodic() << endl;
		outfile << lattice.isZPeriodic() << endl;
		outfile << (int)Site_types.size() << endl;
		for (int n = 0; n < (int)Site_types.size(); n++) {
			outfile << Domain_sizes[n] << endl;
		}
		for (int n = 0; n < (int)Site_types.size(); n++) {
			outfile << Mix_fractions[n] << endl;
		}
		if (!enable_export_compressed) {
			for (int x = 0; x < lattice.getLength(); x++) {
				for (int y = 0; y < lattice.getWidth(); y++) {
					for (int z = 0; z < lattice.getHeight(); z++) {
						outfile << x << "," << y << "," << z << "," << (int)lattice.getSiteType(x, y, z) << endl;
					}
				}
			}
		}
		else {
			int count = 1;
			char previous_type = lattice.getSiteType(0, 0, 0);
			for (int x = 0; x < lattice.getLength(); x++) {
				for (int y = 0; y < lattice.getWidth(); y++) {
					for (int z = 0; z < lattice.getHeight(); z++) {
						if (x == 0 && y == 0 && z == 0) {
							continue;
						}
						if (lattice.getSiteType(x, y, z) == previous_type) {
							count++;
						}
						else {
							outfile << (int)previous_type << count << "\n";
							count = 1;
							previous_type = lattice.getSiteType(x, y, z);
						}
					}
				}
			}
			outfile << (int)previous_type << count << "\n";
		}
	}

	void Morphology::outputTortuosityMaps(ofstream& outfile) const {
		int index;
		outfile << "X-Position,Y-Position";
		for (int n = 0; n < (int)Site_types.size(); n++) {
			outfile << ",Tortuosity" << (int)Site_types[n];
		}
		outfile << endl;
		for (int x = 0; x < lattice.getLength(); x++) {
			for (int y = 0; y < lattice.getWidth(); y++) {
				index = lattice.getWidth()*x + y;
				outfile << x << "," << y;
				for (int n = 0; n < (int)Site_types.size(); n++) {
					outfile << "," << Tortuosity_data[n][index];
				}
				outfile << endl;
			}
		}
	}

	void Morphology::shrinkLattice(int rescale_factor) {
		// Error handling
		if (rescale_factor == 0) {
			cout << "Error! Lattice cannot be shrunken by a rescale factor of zero." << endl;
			throw invalid_argument("Error! Lattice cannot be shrunken by a rescale factor of zero.");
		}
		if (lattice.getLength() % rescale_factor != 0 || lattice.getWidth() % rescale_factor != 0 || lattice.getHeight() % rescale_factor != 0) {
			cout << "Error! All lattice dimensions are not divisible by the rescale factor." << endl;
			throw invalid_argument("Error! All lattice dimensions are not divisible by the rescale factor.");
		}
		// Clear existing Site_type_counts data
		for (int n = 0; n < (int)Site_types.size(); n++) {
			Site_type_counts[n] = 0;
		}
		// Construct the smaller lattice 
		Lattice lattice_rescale = lattice;
		lattice_rescale.resize(lattice.getLength() / rescale_factor, lattice.getWidth() / rescale_factor, lattice.getHeight() / rescale_factor);
		// Assign site types to the new lattice based on the existing lattice
		int type1_count;
		bool alternate = true;
		char target_site_type;
		for (int x = 0; x < lattice_rescale.getLength(); x++) {
			for (int y = 0; y < lattice_rescale.getWidth(); y++) {
				for (int z = 0; z < lattice_rescale.getHeight(); z++) {
					type1_count = 0;
					for (int i = rescale_factor * x; i < (rescale_factor*x + rescale_factor); i++) {
						for (int j = rescale_factor * y; j < (rescale_factor*y + rescale_factor); j++) {
							for (int k = rescale_factor * z; k < (rescale_factor*z + rescale_factor); k++) {
								if (lattice.getSiteType(i, j, k) == (char)1) {
									type1_count++;
								}
							}
						}
					}
					if (2 * type1_count > (rescale_factor*rescale_factor*rescale_factor)) {
						target_site_type = (char)1;
					}
					else if (2 * type1_count < (rescale_factor*rescale_factor*rescale_factor)) {
						target_site_type = (char)2;
					}
					else {
						if (alternate) {
							target_site_type = (char)1;
						}
						else {
							target_site_type = (char)2;
						}
						alternate = !alternate;
					}
					lattice_rescale.setSiteType(x, y, z, target_site_type);
					Site_type_counts[getSiteTypeIndex(target_site_type)]++;
				}
			}
		}
		// Update the lattice
		lattice = lattice_rescale;
		// The shrinking process can change the mix fraction, so the Mix_fraction property is updated.
		calculateMixFractions();
	}

	void Morphology::stretchLattice(int rescale_factor) {
		if (rescale_factor == 0) {
			cout << "Error! Lattice cannot be stretched by a rescale factor of zero." << endl;
			throw invalid_argument("Error! Lattice cannot be streched by a rescale factor of zero.");
		}
		// Clear existing Site_type_counts data
		for (int n = 0; n < (int)Site_types.size(); n++) {
			Site_type_counts[n] = 0;
		}
		// Construct the larger lattice
		Lattice lattice_rescale = lattice;
		lattice_rescale.resize(lattice.getLength()*rescale_factor, lattice.getWidth()*rescale_factor, lattice.getHeight()*rescale_factor);
		// Assign site types to the new lattice based on the existing lattice
		for (int x = 0; x < lattice.getLength(); x++) {
			for (int y = 0; y < lattice.getWidth(); y++) {
				for (int z = 0; z < lattice.getHeight(); z++) {
					for (int i = rescale_factor * x; i < rescale_factor*x + rescale_factor; i++) {
						for (int j = rescale_factor * y; j < rescale_factor*y + rescale_factor; j++) {
							for (int k = rescale_factor * z; k < rescale_factor*z + rescale_factor; k++) {
								lattice_rescale.setSiteType(i, j, k, lattice.getSiteType(x, y, z));
								Site_type_counts[getSiteTypeIndex(lattice.getSiteType(x, y, z))]++;
							}
						}
					}
				}
			}
		}
		// Update the lattice
		lattice = lattice_rescale;
		// The stretch process can change the mix fraction, so the Mix_fraction property is updated.
		calculateMixFractions();
	}

	double Morphology::rand01() {
		return generate_canonical<double, std::numeric_limits<double>::digits>(gen);
	}

	void Morphology::setParameters(const Parameters& params) {
		if (!params.checkParameters()) {
			cout << ID << ": Error! Input parameters are invalid." << endl;
			throw invalid_argument("Error! Input parameters are invalid.");
		}
		Params = params;
	}

	void Morphology::updateNeighborCounts(long int site_index1, long int site_index2) {
		char site_type1 = lattice.getSiteType(site_index1);
		char site_type2 = lattice.getSiteType(site_index2);
		long int neighbor_index;
		Neighbor_counts[site_index1] = Temp_counts1;
		Neighbor_counts[site_index2] = Temp_counts2;
		for (int i = 0; i < 6; i++) {
			neighbor_index = Neighbor_info[site_index1].first_indices[i];
			if (neighbor_index >= 0 && neighbor_index != site_index2) {
				if (lattice.getSiteType(neighbor_index) == site_type1) {
					Neighbor_counts[neighbor_index].sum1++;
				}
				else {
					Neighbor_counts[neighbor_index].sum1--;
				}
			}
		}
		for (int i = 0; i < 6; i++) {
			neighbor_index = Neighbor_info[site_index2].first_indices[i];
			if (neighbor_index >= 0 && neighbor_index != site_index1) {
				if (lattice.getSiteType(neighbor_index) == site_type2) {
					Neighbor_counts[neighbor_index].sum1++;
				}
				else {
					Neighbor_counts[neighbor_index].sum1--;
				}
			}
		}
		for (int i = 0; i < 12; i++) {
			neighbor_index = Neighbor_info[site_index1].second_indices[i];
			if (neighbor_index >= 0) {
				if (lattice.getSiteType(neighbor_index) == site_type1) {
					Neighbor_counts[neighbor_index].sum2++;
				}
				else {
					Neighbor_counts[neighbor_index].sum2--;
				}
			}
		}
		for (int i = 0; i < 12; i++) {
			neighbor_index = Neighbor_info[site_index2].second_indices[i];
			if (neighbor_index >= 0) {
				if (lattice.getSiteType(neighbor_index) == site_type2) {
					Neighbor_counts[neighbor_index].sum2++;
				}
				else {
					Neighbor_counts[neighbor_index].sum2--;
				}
			}
		}
		//if (Enable_third_neighbor_interaction) {
		//	for (int i = 0; i < 8; i++) {
		//		neighbor_index = Neighbor_info[site_index1].third_indices[i];
		//		if (neighbor_index >= 0 && neighbor_index != site_index2) {
		//			if (lattice.getSiteType(neighbor_index) == site_type1) {
		//				Neighbor_counts[neighbor_index].sum3++;
		//			}
		//			else {
		//				Neighbor_counts[neighbor_index].sum3--;
		//			}
		//		}
		//	}
		//	for (int i = 0; i < 8; i++) {
		//		neighbor_index = Neighbor_info[site_index2].third_indices[i];
		//		if (neighbor_index >= 0 && neighbor_index != site_index1) {
		//			if (lattice.getSiteType(neighbor_index) == site_type2) {
		//				Neighbor_counts[neighbor_index].sum3++;
		//			}
		//			else {
		//				Neighbor_counts[neighbor_index].sum3--;
		//			}
		//		}
		//	}
		//}
	}
}
