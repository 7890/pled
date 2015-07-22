// ----------------------------------------------------------------------------
//
//  Copyright (C) 2015 Thomas Brand <tom@trellis.ch>
//    
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//  This file is part of pled
//  https://github.com/7890/pled
//
//tb/150716+
// ----------------------------------------------------------------------------

#ifndef FILES_H_INCLUDED
#define FILES_H_INCLUDED

#define MAX_FILES 1000

//incremented at struct creation
uint64_t FileNode_serial_id_counter=0;

//===============================================================================
//===============================================================================
struct FileNode
{
	uint64_t id_serial;

	string uri;

	uint64_t offset;
	uint64_t count;

	//=======================================================================
	FileNode(string full_path_to_file, uint64_t offset_, uint64_t count_)
	{
		id_serial=FileNode_serial_id_counter;
		FileNode_serial_id_counter++;

		uri=full_path_to_file;
		offset=offset_;///////
		count=count_;
	}

	//=======================================================================
	void print(uint64_t index)
	{
		fprintf(stderr,"[%"PRId64"] '%s' %"PRId64" %"PRId64"\n"
			,index,uri.c_str(),offset,count);
	}
};//end struct FileNode

//===============================================================================
//===============================================================================
struct Files
{
	struct FileNode *fnodes[MAX_FILES];

	uint64_t file_counter;

	//=======================================================================
	void reset()
	{
		for(int i=0;i<MAX_FILES;i++)
		{
			fnodes[i]=NULL;
		}
		file_counter=0;
		//default
		add(new FileNode("/dev/null",0,0));
	}

	//=======================================================================
	Files()
	{
		reset();
	}

	//=======================================================================
	uint64_t add(FileNode *fn)
	{
		fnodes[file_counter]=fn;
		file_counter++;
		return file_counter;
	}

	//=======================================================================
	int is_empty()
	{
		if(file_counter==0)
		{
			return 1;
		}
		return 0;
	}

	//=======================================================================
	uint64_t count()
	{
		return file_counter;
	}

	//=======================================================================
	FileNode * get_at_index(uint64_t index)
	{
		if(index>=0 && index<file_counter)
		{
			return fnodes[index];
		}
		else
		{
			return NULL;
		}
	}

	//=======================================================================
	void print()
	{
		if(file_counter==0)
		{
			fprintf(stderr,"//===filelist: empty\n");
			return;
		}

		//header
		fprintf(stderr,"//===filelist: %"PRId64" items\n//[index] 'uri' offset count\n"
			,file_counter
		);

		for(uint64_t i=0;i<file_counter;i++)
		{
			fnodes[i]->print(i);
		}
		fprintf(stderr,"\n");
	}
};//end struct Files

#endif
//EOF
