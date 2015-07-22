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

#ifndef TREENODE_H_INCLUDED
#define TREENODE_H_INCLUDED

#define MAX_FILES 1000

#include "uthash/uthash.h"
#include "event.h"

//incremented at struct creation
uint64_t TreeNode_serial_id_counter=0;

//this allows to use TreeNode before struct effectively defined
typedef struct TreeNode TreeNode;

//all nodes created via create_node are put here
struct TreeNode *nodes = NULL;

void hash_node(struct TreeNode *t);
TreeNode *find_node(int id);
TreeNode * create_node();

//===============================================================================
//===============================================================================
//to build binary tree structures, holding events
struct TreeNode
{
	uint64_t id_serial;

	Event *ev;

	TreeNode *left;  //left subtree
	TreeNode *right; //right subtree

	UT_hash_handle hh; //make this structure hashable

	//=======================================================================
	void init()
	{
		id_serial=TreeNode_serial_id_counter;
		TreeNode_serial_id_counter++;

		ev=new Event();

		left=NULL;
		right=NULL;
	}

	//=======================================================================
	TreeNode()
	{
		init();
//		fprintf(stderr,"created TreeNode id %"PRId64"\n",id_serial);
	}

	//=======================================================================
	int is_leaf()
	{
		return (left==NULL && right==NULL);
	}

	//=======================================================================
	int undo()
	{
		if(is_leaf())
		{
			fprintf(stderr,"/!\\ can't undo on leaf node\n");
			return 0;
		}
		left=NULL;
		right=NULL;

		return 1;
	}

	//=======================================================================
	//recursive
	TreeNode * get_first()
	{
		if(is_leaf())
		{
			return this;
		}
		else if(left!=NULL)
		{
			return left->get_first();
		}
		else if(right!=NULL)
		{
			return right->get_first();
		}
	}

	//=======================================================================
	//recursive
	TreeNode * get_last()
	{
		if(is_leaf())
		{
			return this;
		}
		else if(right!=NULL)
		{
			return right->get_last();
		}
		else if(left!=NULL)
		{
			return left->get_last();
		}
	}

	//=======================================================================
	//recursive, remove if leaf and empty
	int can_drop()
	{
		int ret=0;

		if(is_leaf() && ev->is_empty())
		{
			return 1;
		}
		if(left!=NULL)
		{
			if(left->can_drop())
			{
				left=NULL;
			}
		}
		if(right!=NULL)
		{
			if(right->can_drop())
			{
				right=NULL;
			}
		}
		return 0;
	}

	//=======================================================================
	//recursive, context 0: root 1: left 2: right
	void eval(int do_print, int level, uint64_t parent_node_id, int context
		,uint64_t &_linear_counter, uint64_t &_remove_counter, uint64_t &_add_counter)
	{
		if(do_print)
		{
			for(int i=0;i<level;i++)
			{
				fprintf(stderr,"  ");
			}

			fprintf(stderr,"<node id=\"%"PRId64"\" pid=\"%"PRId64"\" level=\"%d\" context=\"%d\" "
				,id_serial,parent_node_id,level,context);

			if(is_leaf())
			{
				fprintf(stderr,"leaf=\"1\">\n");
			}
			else
			{
				fprintf(stderr,"leaf=\"0\">\n");
			}
		}

		ev->eval(do_print,level,is_leaf(),id_serial
			,_linear_counter,_remove_counter,_add_counter);

		if(left!=NULL)
		{
			left->eval(do_print,level+1,id_serial,1
				,_linear_counter,_remove_counter,_add_counter);
		}
		if(right!=NULL)
		{
			right->eval(do_print,level+1,id_serial,2
				,_linear_counter,_remove_counter,_add_counter);
		}

		if(do_print)
		{
			for(int i=0;i<level;i++)
			{
				fprintf(stderr,"  ");
			}

			fprintf(stderr,"</node> <!-- %"PRId64" -->\n",id_serial);
		}
	}//end eval()

	//=======================================================================
	//mother of a lot of methods
	int split_tn(uint64_t pos)
	{
/*
pos must be a value between 0 and ev->count (relative to ev)

start: start pos in file
start_linear: position on final timeline

ev->end()=ev->start+ev->count
ev->end_linear()=ev->start_linear+ev->count


  0      7 (split pos)    100 (ev count)
         v
  |-----------------------|

  42                      142 (ev start + count)
  ev start                ev end

  0                       100 (ev count)
  start lin               end lin



  0---orig node-----------100
         /\
        /  \
       /    \
      /      \
  |------|----------------|

  0      new count=7 (split pos)

         0                93 (new count = orig count - split pos)

  42 (new ev start = orig ev start)

        49 (new ev start = orig ev start + split pos)


  0
  start lin
         7                100
         start lin        end lin


special case split pos=0:

left: count 0 (empty)
right: orig count


special case split pos=count:

left: orig count
right: count 0 (empty)


test cases:

10, split at 4
10, split at 6

10, split at 0
10, split at 10

//outside
10, split at 11

*/

/*
		fprintf(stderr,"split Event id %"PRId64" at pos %"PRId64" in TreeNode id %"PRId64"\n"
			,ev->id_serial
			,pos
			,id_serial
		);
*/

		if(pos<0 || pos>ev->count)
		{
			fprintf(stderr,"/!\\ position %"PRId64" outside range. ev(%"PRId64")->count: %"PRId64"\n"
				,pos,ev->id_serial,ev->count);
			return 0;
		}

		//create left and right subnodes, result of split
		TreeNode *tL = create_node();
		TreeNode *tR = create_node();

		//set new start, count
		tL->ev->start=ev->start;		//new ev start = orig ev start
		tL->ev->count=pos;			//new ev count = split pos

		tR->ev->start=ev->start+pos;		//new ev start = orig ev start + split pos
		tR->ev->count=ev->count-pos;		//new count = orig count - split pos

		//children inherit props
		tL->ev->silent=ev->silent;
		tL->ev->cut=ev->cut;
		tL->ev->additional=ev->additional;
		tL->ev->file_id=ev->file_id;

		tR->ev->silent=ev->silent;
		tR->ev->cut=ev->cut;
		tR->ev->additional=ev->additional;
		tR->ev->file_id=ev->file_id;

		//attach
		left =tL;
		right=tR;
		return 1;
	}//end split_tn()

	//=======================================================================
	int insert_create_node(uint64_t pos, uint64_t count, uint64_t start, uint64_t file_id)
	{
/*
pos >=0, relative to treenode ev 0
linear positions must be offset previously

insert as a series of split ops


       split=pos
       v
|A----------------|


|B-----|
       |C---------|


left, split at end
       v
|B-----|


|D-----|
       |E

       right: set insert count as requested
       |E mod---------------|

/!\\ orig node won't reflect total count of all underlying leaf counts now

                                                  .
          v                                       .
   |A----------------|                            .
          .      o                                .
          .    __|__________                      .
          .   /             \                     .
          .  /               \                    .
            o                 \                   .
           /\                  \                  .
          /  \                  \                 .
         /    \                  \                .
   |D-----|E mod---------------|C---------|       .
          .                                       .
                                                  .

test cases:

insert count: 0, 1

10, insert at 4
10, insert at 6

10, insert at 0
10, insert at 10

//outside
10, insert at 11

*/

		if(!is_leaf())
		{
			fprintf(stderr,"/!\\ insert: is not leaf\n");
/*
			uint64_t _linear_counter=0;
			uint64_t _remove_counter=0;
			uint64_t _add_counter=0;
			eval(1,0,0,0,_linear_counter,_remove_counter,_add_counter);
*/
			return 0;
		}

		if(count==0)
		{
//			fprintf(stderr,"insert: no work to do (count==0)\n");
			return 0;///
		}

		if(!split_tn(pos))
		{
//			fprintf(stderr,"/!\\ insert: split: error 1\n");
			return 0;
		}

		if(!left->split_tn(left->ev->count))
		{
//			fprintf(stderr,"/!\\ insert: split: error 2\n");
			return 0;
		}

		TreeNode *tlr=left->right;

		tlr->ev->count=count;
		tlr->ev->cut=0;

		//file 0: /dev/null
		if(file_id==0)
		{
			tlr->ev->silent=1;
		}
		else
		{
			tlr->ev->silent=0;

			//set orig start
			tlr->ev->start=start;
		}

		tlr->ev->file_id=file_id;
		tlr->ev->additional=1;

		return 1;
	}

	//=======================================================================
	int insert_create_node(uint64_t pos, uint64_t count)
	{
		return insert_create_node(pos,count,0,0);
	}

	//=======================================================================
	uint64_t cut_or_silent_tn(uint64_t pos, uint64_t count, int cut)
	{
/*
cut is: split A, split A-b, throw away A-b-a
must be in focus of this node (i.e. inter-node solved previously)
count will be limited to max possible
returns effectively cut

    v-----|
|A---------------|

|Aa.|Ab
    |------------|
    |Aba--|Abb---|


           pos             pos+count
           v---------------|
|------------------|
           v--|

*/
		if(!split_tn(pos)) //-> Aa, Ab
		{
			return 0;
		}

		uint64_t count_max=ev->count;
		uint64_t count_max_do=count_max-pos;
		uint64_t count_do=MIN(count_max_do,count);

		if(!right->split_tn(count_do)) //-> Aba, Abb
		{
			return 0;
		}

		TreeNode *trl=right->left; //Aba

		if(cut)
		{
			trl->ev->cut=1;
		}
		else
		{
			trl->ev->silent=1;
		}

		return count_do;
	}//end cut_silent_tn()

	//=======================================================================
	uint64_t cut_tn(uint64_t pos, uint64_t count)
	{
		return cut_or_silent_tn(pos,count,1);
	}

	//=======================================================================
	uint64_t silent_tn(uint64_t pos, uint64_t count)
	{
		return cut_or_silent_tn(pos,count,0);
	}
};//end struct TreeNode

//===============================================================================
void hash_node(struct TreeNode *t)
{
	HASH_ADD_INT( nodes, id_serial, t );
}

//===============================================================================
TreeNode *find_node(int id)
{
	TreeNode *t;
	HASH_FIND_INT( nodes, &id, t );
	return t;
}

//===============================================================================
TreeNode * create_node()
{
	TreeNode *t=new TreeNode();
	hash_node(t);
	return t;
}

#endif
//EOF
