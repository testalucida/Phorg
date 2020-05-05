/*
 * PhotoInfo.h
 *
 *  Created on: 05.05.2020
 *      Author: martin
 */

#ifndef PHOTOINFO_H_
#define PHOTOINFO_H_

#include <string>

class PhotoBox;

struct PhotoInfo {
	std::string folder;
	std::string filename;
	std::string datetime;
	PhotoBox* box = NULL;
};



#endif /* PHOTOINFO_H_ */
