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

#ifndef TIMELINE_H_INCLUDED
#define TIMELINE_H_INCLUDED

#include "playlist.h"
#include "files.h"

using namespace std;

//hack
Playlist *pl;

//===============================================================================
//===============================================================================
struct Timeline
{
	TreeNode *root;
	Ops *ops;
	Files *files;

	MergedPlaylist *pl_merged;

	string id;

	uint64_t linear_counter;
	uint64_t remove_counter;
	uint64_t add_counter;

	//=======================================================================
	void reset_counters()
	{
		linear_counter=0;
		remove_counter=0;
		add_counter=0;
	}

	//=======================================================================
	//always starts at 0
	Timeline(string s)
	{
		id=s;
		reset_counters();

		root=create_node();

		ops=new Ops();

		files=new Files();

		pl=new Playlist();
		pl_merged=NULL;

		//init
		eval(0);
	}

	//=======================================================================
	int exec(Op *op, int new_op)
	{
		if(new_op && op->id!=UNDO_OP && op->id!=REDO_OP)
		{
			ops->all_redone();
		}

		if(op->id==PLAYLIST_OP)
		{
///
		}
		else if(op->id==FILE_OP)
		{
			file_((FileOp*)op);
		}
		else if(op->id==INSERT_OP)
		{
			insert((InsertOp*)op);
		}
		else if(op->id==INSERT_FILE_OP)
		{
			insert_file((InsertFileOp*)op);
		}
		else if(op->id==CUT_OP)
		{
			cut((CutOp*)op);
		}
		else if(op->id==SILENT_OP)
		{
			silent((SilentOp*)op);
		}
		else if(op->id==UNDO_OP)
		{
			undo();
		}
		else if(op->id==REDO_OP)
		{
			redo();
		}
		else if(op->id==NUMERIC_OP)
		{
			numeric((NumericOp*)op);
		}
		else
		{
			fprintf(stderr,"/!\\ unknown op\n");
			return 0;
		}
		return 1;

	}//end exec()

	//=======================================================================
	int exec(Op *op)
	{
		return exec(op,1);
	}

	//=======================================================================
	//recursive, after every op to update
	int eval(int do_print)
	{
		pl->reset();
		reset_counters();

		//simplify
		root->can_drop();

		///
		int do_print_xml=0;
		if(do_print_xml)
		{
			fprintf(stderr,"<playlist id=\"%s\" count=\"%"PRId64"\">\n"
				,id.c_str()
				,root->ev->count
			);
		}

		root->eval(do_print_xml,0,0,0
			,linear_counter,remove_counter,add_counter);

		if(do_print_xml)
		{
			fprintf(stderr,"<playlist/>\n\n");
		}

		if(do_print)
		{
			ops->print();
			files->print();
//			pl->print();

			if(pl_merged==NULL)
			{
				pl_merged=new MergedPlaylist(pl);
			}
			else
			{
				pl_merged->update(pl);
			}
			pl_merged->print();
		}
	}//end eval()

	//=======================================================================
	int file_(FileOp *o)
	{
		string file_uri=o->uri;
		uint64_t offset=o->args[0];
		uint64_t count=o->args[1];

		FileNode *fn=new FileNode(file_uri,offset,count);
		files->add(fn);

		ops->add(new FileOp(file_uri,offset,count));

//		files->print();

		return 1;
	}//end file_()

	//=======================================================================
	int insert_file(InsertFileOp *o)
	{
		uint64_t pos=o->args[0];
		uint64_t file_index=o->args[1];

		FileNode *filenode = files->get_at_index(file_index);

		if(filenode==NULL)
		{
			fprintf(stderr,"/!\\ filenode null\n");
			return 0;
		}
		//copy
		InsertFileOp *op=new InsertFileOp(pos,file_index);

		//leaf node on wich inert op will be carried out
		TreeNode *insert;

		int index_from=pl->find_pos_index(pos,1);

		if(index_from<0)
		{
			index_from=pl->find_pos_index(pos,0);
		}

		if(index_from<0)
		{
			index_from=0;//root
			insert=root;
		}
		else
		{
			insert=pl->get_node_at_index(index_from);
		}

		//make pos relative to node
		pos-=insert->ev->start_linear;

		insert->insert_create_node
			(pos,filenode->count,filenode->offset,file_index);

		//set involved node ids
		op->add_node_id(insert->id_serial);
		//now add to ops
		ops->add(op);

		eval(0);
		return 1;
	}//end insert_file()

	//=======================================================================
	//insert always only affects one node
	int insert(InsertOp *o)
	{
		uint64_t pos=o->args[0];
		uint64_t count=o->args[1];

		//copy
		InsertOp *op=new InsertOp(pos,count);

		//leaf node on wich inert op will be carried out
		TreeNode *insert;

		int index_from=pl->find_pos_index(pos,1);

		if(index_from<0)
		{
			index_from=pl->find_pos_index(pos,0);
		}

		if(index_from<0)
		{
			index_from=0;//root
			insert=root;
		}
		else
		{
			insert=pl->get_node_at_index(index_from);
		}
/*
		fprintf(stderr,"pos %"PRId64" count %"PRId64" index from %d\n"
			,pos,count
			,index_from);
*/

		//make pos relative to node
		pos-=insert->ev->start_linear;

		insert->insert_create_node(pos,count);

		//set involved node ids
		op->add_node_id(insert->id_serial);
		//now add to ops
		ops->add(op);

		eval(0);
		return 1;
	}//end insert()

	//=======================================================================
	//can involve multiple nodes
	int cut_or_silent(Op * o, int cut)
	{
		uint64_t pos=o->args[0];
		uint64_t count=o->args[1];

		//copy
		Op *op;
		if(cut)
		{
			op=new CutOp(pos,count);
		}
		else
		{
			op=new SilentOp(pos,count);
		}

		//leaf node on wich inert op will be carried out
		TreeNode *cut_start_node;

		int index_from=pl->find_pos_index(pos,1);

		if(index_from<0)
		{
			index_from=pl->find_pos_index(pos,0);
		}

		if(index_from<0)
		{
			index_from=0;//root
			cut_start_node=root;
		}
		else
		{
			cut_start_node=pl->get_node_at_index(index_from);
		}
/*
		fprintf(stderr,"pos %"PRId64" count %"PRId64" index from %d\n"
			,pos,count
			,index_from);
*/

/*
                cut pos(lin)        pos lin + count   
timeline        v                   |
|------------------------------------------------------>
         |X--------|Y----------|Z------------| involved leafs (in plitems)

|A-------|B-----|C-|Y-cut=1----|D---|E-------|
                 cut=1          cut=1

                |-------------------|cut remain

cut pos relative: cut pos lin - X start linear

A count: X start linear
B count: cut pos rel
C count: X count - cut pos rel

*/

		//make pos relative to node
		uint64_t pos_=pos;
		pos_-=cut_start_node->ev->start_linear;

		uint64_t count_remain=count;

		TreeNode *tn=cut_start_node;
		int index=index_from;

		while(count_remain>0)
		{
			if(op!=NULL)
			{
				//add involved node id
				op->add_node_id(tn->id_serial);
			}

			if(cut)
			{
				count_remain-=tn->cut_tn(pos_,count_remain);
			}
			else//silent
			{
				count_remain-=tn->silent_tn(pos_,count_remain);
			}

			index++;
			pos_=0;

			tn=pl->get_node_at_index(index);

			if(tn==NULL)
			{
				break;
			}
		}

		if(count_remain>0)
		{
			fprintf(stderr,"/!\\ cut remaining frames %"PRId64"\n",count_remain);
		}

		//now add to ops
		ops->add(op);

		eval(0);
		return 1;
	}//end cut_or_silent()

	//=======================================================================
	int cut(CutOp * o)
	{
		return cut_or_silent(o,1);
	}

	//=======================================================================
	int silent(SilentOp * o)
	{
		return cut_or_silent(o,0);
	}

	//=======================================================================
	void undo()
	{
		ops->undo();
		eval(0);
	}
	//=======================================================================
	void redo()
	{
		Op * redo=ops->get_redo_op();
		if(redo!=NULL)
		{
			ops->prepare_redo();
			exec(redo,0);
		}
	}

	//=======================================================================
	void numeric(NumericOp *o)
	{
		Op * numeric_redo=ops->get_op_at_index(o->args[0]);
		if(numeric_redo!=NULL)
		{
			exec(numeric_redo);
		}
	}

};//end struct Timeline

//wrapper called from Event
//===============================================================================
int add_to_playlist(uint64_t start_linear, uint64_t nid, uint64_t start, uint64_t count, int silent, uint64_t file_id)
{
	if(pl==NULL)
	{
		return 0;
	}
	return pl->add(start_linear,nid,start,count,silent,file_id);
}

#endif 
//EOF
