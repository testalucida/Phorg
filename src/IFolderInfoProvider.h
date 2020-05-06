/*
 * IFolderInfoProvider.h
 *
 *  Created on: 06.05.2020
 *      Author: martin
 */

#ifndef IFOLDERINFOPROVIDER_H_
#define IFOLDERINFOPROVIDER_H_

#include <memory>

class IFolderInfoProvider {
public:
	/** to provide burger menu with menu items */
	virtual void getMoveToSubfolders( std::vector<string>& ) = 0;
};



#endif /* IFOLDERINFOPROVIDER_H_ */
