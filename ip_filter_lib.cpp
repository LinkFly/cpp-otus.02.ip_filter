#include <iostream>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <iterator>
#include <functional>
#include <fstream>
#include <sstream>
//for debug
//#include <typeinfo>

#include "ip_filter_lib.h"
#include "build/config.h"

using std::cout;
using std::endl;

std::vector<std::string> split(const std::string& str, char d)
{
	std::vector<std::string> r;

	std::string::size_type start = 0;
	std::string::size_type stop = str.find_first_of(d);
	while (stop != std::string::npos)
	{
		r.push_back(str.substr(start, stop - start));

		start = stop + 1;
		stop = str.find_first_of(d, start);
	}

	r.push_back(str.substr(start));

	return r;
}

void read_lines(std::istream& stream, func_str fn_line_handler) {
	for (std::string line; std::getline(stream, line);)
		fn_line_handler(line);
}

////////////////////////////////////////////////////

bool check_started_1(const vecint& ip_parts) {
	return ip_parts[0] == 1;
}

bool check_started_46_70(const vecint& ip_parts) {
	return ip_parts[0] == 46 && ip_parts[1] == 70;
}

bool check_includes_46(const vecint& ip_parts) {
	for (auto ip_part : ip_parts) {
		if (ip_part == 46)
			return true;
	}
	return false;
}

//template<class T>
//void output_pools(std::ostream& out, std::vector<ip_pool<T>> pools, std::function<std::string(T)> fn_prepare_ip);

//std::string unpack_ip(vecint& ip_parts) {
//	std::ostringstream ostr;
//	for (auto it = ip_parts.cbegin(); it != ip_parts.cend(); it++) {
//		if (it != ip_parts.cbegin()) {
//			ostr << ".";
//		}
//		ostr << *it;
//	}
//	return ostr.str();
//}

template<typename T>
void PoolCollection<T>::add_from_line(ip_pool<T>& ip_pool, std::string& line) {
	vecstr v = split(line, '\t');
	vecstr snums = split(v.at(0), '.');
	vecint ip_parts;
	for (auto snum : snums) { ip_parts.push_back(stoi(snum)); }
	ip_pool.push_back(ip_parts);
}

template<class T>
void PoolCollection<T>::base_sort() {
	pool_sort(base_pool);
}

template<class T>
void PoolCollection<T>::pool_sort(ip_pool<T>& ip_pool) {
	std::sort(ip_pool.begin(), ip_pool.end(), [](const T& vs_left, const T& vs_right) -> bool {
		for (int i = 0; i < 4; i++) {
			int num_left = vs_left[i];
			int num_right = vs_right[i];
			if (num_left != num_right) {
				return num_left > num_right;
			}
		}
		return false;
		});
}

template<class T>
void PoolCollection<T>::classify() {
	for (auto ip_parts : base_pool) {
		if (check_started_46_70(ip_parts))
			ip_pool_started_46_70.push_back(ip_parts);
		if (check_includes_46(ip_parts))
			ip_pool_includes_46.push_back(ip_parts);
		if (check_started_1(ip_parts))
			ip_pool_started_1.push_back(ip_parts);
	}
}

template<class T>
std::vector<typename PoolCollection<T>::ip_pool_ptr> PoolCollection<T>::get() {
	return { &base_pool, &ip_pool_started_1, &ip_pool_started_46_70, &ip_pool_includes_46 };
}

template<typename T>
std::string PoolCollection<T>::unpack_ip(const T& ip_parts) {
	std::ostringstream ostr;
	for (auto it = ip_parts.cbegin(); it != ip_parts.cend(); it++) {
		if (it != ip_parts.cbegin()) {
			ostr << ".";
		}
		ostr << *it;
	}
	return ostr.str();
}

template<typename T>
void PoolCollection<T>::output_pools(std::ostream& out, const std::vector<PoolCollection::ip_pool_ptr>& pools) {
	for (auto *cur_pool : pools) {
		for (T ip_desc : *cur_pool) {
			/*std::string ip = fn_prepare_ip(ip_desc);*/
			std::string ip = unpack_ip(ip_desc);
			out << ip << "\n";
		}
	}
}

// TODO! Don't call unpack_ip (make special structure)
template<typename T>
void PoolCollection<T>::filtering_and_output_pools(std::ostream& out) {
	ip_pool<T>& pool = base_pool;

	// Output sorted base_pool
	output_pools(out, std::vector{ &pool });

	// Helpers
	auto fnOutputIp = [this, &out](auto cur_ip) {
		std::string sIP = this->unpack_ip(cur_ip);
		out << sIP << endl;
	};
	auto fnOutputIpRange = [&pool, &fnOutputIp](auto itFirst, auto itLast) {
		std::for_each(itFirst, itLast, fnOutputIp);
	};

	// Output ip which started 1
	auto itFirst1 = find_if_not(std::reverse_iterator(pool.rbegin()), pool.rend(), check_started_1).base();
	fnOutputIpRange(itFirst1, pool.end());

	// Output ip which started 46.70
	auto fnFirstEq46 = [](const T& ip_parts) {
		return ip_parts[0] == 46;
	};
	auto fnSecondEq70 = [](const T& ip_parts) {
		return ip_parts[1] == 70;
	};
	auto fnExists46 = [](const T& ip_parts) {
		return any_of(ip_parts.begin(), ip_parts.end(),
			[](int part) {return part == 46; });
	};
	auto itFirst46 = find_if(pool.begin(), pool.end(), fnFirstEq46);
	auto itFirstNot46 = find_if_not(itFirst46, pool.end(), fnFirstEq46);
	auto itCurrent = itFirst46;
	auto itFirstSecond70 = find_if(itCurrent, itFirstNot46, fnSecondEq70);
	auto itFirstSecondNot70 = find_if_not(itFirstSecond70 + 1, itFirstNot46, fnSecondEq70);
	fnOutputIpRange(itFirstSecond70, itFirstSecondNot70);

	// Output ip which contained 46
	std::for_each(pool.begin(), pool.end(), [this, &out, &fnExists46](const T& ip_parts) {
		if (fnExists46(ip_parts))
			out << this->unpack_ip(ip_parts) << endl;
		});
}


void run(std::istream &in, std::ostream &out) {
	using PoolCol = PoolCollection<vecint>;
	PoolCol ip_pools_col;

	read_lines(in, [&ip_pools_col](std::string line) {
		PoolCol::add_from_line(ip_pools_col.base_pool, line);
	});

	//  reverse lexicographically sort
	ip_pools_col.base_sort();

	if (!ENABLED_OPTIMIZE_MEMORY) {
		// Prepare classified pools
		ip_pools_col.classify();
		//// Output
		ip_pools_col.output_pools(out, ip_pools_col.get());
	}
	else {
		ip_pools_col.filtering_and_output_pools(out);
	}


	/*auto ip_pools = ip_pools_col.get();
	output_pools<vecint>(out, ip_pools, unpack_ip);*/
}
