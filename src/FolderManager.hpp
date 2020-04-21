/*
 * FolderManager.hpp
 *
 *  Created on: 21.04.2020
 *      Author: martin
 */

#ifndef FOLDERMANAGER_HPP_
#define FOLDERMANAGER_HPP_

#include "std.h"
#include <my/StringHelper.h>
#include <FL/Fl_Native_File_Chooser.H>
#include <stdexcept>
#include <algorithm>
#include <unistd.h>

using namespace std;

enum Sort {
	SORT_ASC,
	SORT_DESC,
	SORT_NONE
};

static Sort _sortDirection = Sort::SORT_ASC;

struct ImageInfo {
	string folder;
	string filename;
	string datetime;
};

class FolderManager {
public:
	FolderManager() {
		_fileChooser.title( "Choose folder with photos to show" );
		_fileChooser.type( Fl_Native_File_Chooser::BROWSE_DIRECTORY );
	}

	~FolderManager() {}

	const char* chooseFolder() {
		// Show file chooser
		if( !_folder.empty() ) {
			_fileChooser.directory( _folder.c_str() );
		}
		switch ( _fileChooser.show() ) {
		case -1: /*ERROR -- todo*/
			;
			break; // ERROR
		case 1: /*CANCEL*/
			;
			fl_beep();
			break; // CANCEL
		default: // PICKED DIR
			_folder.clear();
			_folder.append( _fileChooser.filename() );
			return _folder.c_str();
		}

		return NULL;
	}

	/**
	 * Gets all jpg, jpeg JPG and JPEG files in the given folder.
	 * No png files.
	 * Rotate jpeg if wanted and necessary and sort them if wanted
	 * ascending or descending according to  Date/Time info in the EXIF block.
	 * This is done by using jhead.
	 * Note:
	 * By each call of this method all PathnFile elements
	 * of the previously returned vector  will be deleted.
	 */
	vector<ImageInfo*>& getImages( const char* folder,
			                       bool rotate = true,
								   Sort sort = Sort::SORT_ASC )
    {
		clearImages();

		if( rotate ) {
			rotateImages( folder );
		}

		collectImageInfos( folder );

		if( sort != Sort::SORT_NONE ) {
			sortImages( sort );
		}

		return _images;
	}

private:
	bool isImageFile( const char *filename ) {
		my::StringHelper &strh = my::StringHelper::instance();
		if ( strh.endsWith( filename, "jpg" )
				//|| strh.endsWith( filename, "png" )
				//|| strh.endsWith( filename, "PNG" )
				|| strh.endsWith( filename, "jpeg" )
				|| strh.endsWith( filename, "JPG" )
				|| strh.endsWith( filename, "JPEG " ) )
		{
			return true;
		}

		return false;
	}

	void clearImages() {
		for( auto img : _images ) {
			delete img;
		}
		_images.clear();
	}

	void rotateImages( const char* folder ) {
		string command = "jhead ";
		command.append( "-autorot " );
		command.append( folder );
		command.append( "/*.*" );
		int rc = system( command.c_str() );
		if( rc != 0 ) {
			throw runtime_error( "FolderManager::rotate(): Rotation failed." );
		}
	}

	void collectImageInfos( const char* folder ) {
		string command = "jhead ";
		command.append( folder );
		command.append( "/*.*" );
		FILE* pipe;
		pipe = popen( command.c_str(), "r" );
		if (!pipe) throw std::runtime_error( "FolderManager::collectImageInfos(): popen() failed!" );
		char buffer[128];
		string result = "";
		try {
			while (fgets( buffer, sizeof buffer, pipe ) != NULL) {
				result += buffer;
			}
		} catch (...) {
			pclose( pipe );
			throw;
		}
		pclose( pipe );

		provideImageInfos( result );
	}

	void getDateTimeInfo( const string& result, string& datetime, size_t start ) {
		const string datetime_const = "Date/Time";
		size_t pos1 = result.find( datetime_const, start );
		if( pos1 != string::npos ) {
			size_t posX = result.find( ":", pos1 ) + 1;
			if( posX != string::npos ) {
				size_t posEOL = result.find( "\n", posX );
				if( posEOL != string::npos ) {
					int len = posEOL - posX;
					datetime = result.substr( posX, len );
				} else {
					throw runtime_error( "FolderManager::getDateTimeInfo(): "
										 "Can't find end of line" );
				}
			} else {
				throw runtime_error( "FolderManager::getDateTimeInfo(): "
									 "Can't find colon" );
			}
		} else {
			throw runtime_error( "FolderManager::getDateTimeInfo(): "
								 "Can't find Date/Time string" );
		}
	}

	void splitPathnfile( const string& pathnfile, string& path, string& file ) {
		size_t pos = pathnfile.rfind( "/" );
		path = pathnfile.substr( 0, pos );
		int len = pathnfile.size() - pos - 1;
		file = pathnfile.substr( pos+1, len );
	}

	void provideImageInfos( string& result ) {
		const string filename = "File name";

		size_t pos1 = result.find( filename, 0 );
		while( pos1 != string::npos ) {
			size_t posSlash = result.find( "/", pos1 + 1 );
			size_t posEOL;
			if( posSlash != string::npos ) {
				posEOL = result.find( "\n", posSlash + 1 );
				if( posEOL != string::npos ) {
					ImageInfo* ii = new ImageInfo;
					int len = posEOL - posSlash;
					string pathnfile( result, posSlash, len );
					splitPathnfile( pathnfile, ii->folder, ii->filename );
					getDateTimeInfo( result, ii->datetime, posEOL + 1 );
					_images.push_back( ii );
				} else {
					throw runtime_error( "FolderManager::provideImageInfos(): "
										 "Can't find end of line" );
				}
			} else {
				throw runtime_error( "FolderManager::provideImageInfos(): "
									 "Can't find start of path" );
			}
			pos1 = result.find( filename, posEOL + 1 );
		} // while
	}

	void sortImages( Sort sortDirection ) {
		_sortDirection = sortDirection;
		sort( _images.begin(), _images.end(), compare );
	}

	static bool compare( const ImageInfo* i1, const ImageInfo* i2 ) {
		if( i1->datetime > i2->datetime ) {
			return _sortDirection == Sort::SORT_ASC ? false : true;
		}
		if( i1->datetime < i2->datetime ) {
			return _sortDirection == Sort::SORT_ASC ? true : false;
		}
		return ( i1->filename < i2->filename );
	}

private:
	Fl_Native_File_Chooser _fileChooser;
	std::string _folder;
	std::vector<ImageInfo*> _images;
};



#endif /* FOLDERMANAGER_HPP_ */
