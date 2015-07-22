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

#ifndef OPS_H_INCLUDED
#define OPS_H_INCLUDED

#include "treenode.h"

#define PLAYLIST_OP 0
#define INSERT_OP 1
#define CUT_OP 2
#define SILENT_OP 3
#define UNDO_OP 4
#define REDO_OP 5
#define NUMERIC_OP 6
#define FILE_OP 7
#define INSERT_FILE_OP 8

#define MAX_OPS 10000
#define MAX_OP_ARGS 5

#define MAX_NODES_PER_OP 1000

//incremented at struct creation
uint64_t Op_id_serial_counter=0;

using namespace std;

//===============================================================================
//===============================================================================
struct Op
{
	uint64_t id_serial; //increment
	int id; //static, per command

	string name;
	string regex_match_line;
	uint64_t args[MAX_OP_ARGS];
	int args_used;

	virtual void init() = 0;

	virtual void print(uint64_t index) = 0;

	uint64_t node_ids[MAX_NODES_PER_OP];
	uint64_t node_id_list_counter;

	//=======================================================================
	Op()
	{
		id_serial=Op_id_serial_counter;
		Op_id_serial_counter++;

		node_id_list_counter=0;

		name="unknown_op";
		string regex_match_line="*";
		args_used=0;
	}

	//=======================================================================
	int is_match(string in)
	{
		return (! re(in,regex_match_line));
	}

	//=======================================================================
	uint64_t add_node_id(uint64_t nid)
	{
		node_ids[node_id_list_counter]=nid;
		node_id_list_counter++;
		return node_id_list_counter;
	}

	//=======================================================================
	virtual void undo()
	{
		for(int i=0;i<node_id_list_counter;i++)
		{
			TreeNode *t=find_node(node_ids[i]);
			if(t!=NULL)
			{
				t->undo();
			}
		}
	}

	//=======================================================================
	void print_node_ids(int endline)
	{
		for(int i=0;i<node_id_list_counter;i++)
		{
			fprintf(stderr," %"PRId64"%s",node_ids[i]
				,(i==-1+node_id_list_counter) ? "" : ",");
		}

		if(endline)
		{
			fprintf(stderr,"\n");
		}
	}

	//=======================================================================
	void print_common(uint64_t index, int endline)
	{
		if(args_used>MAX_OP_ARGS)
		{
			fprintf(stderr,"/!\\ to many args for cmd: ");
		}

		fprintf(stderr,"[%"PRId64"] %s"
			,index
//			,id_serial
//			,id
			,name.c_str()
			);
		if(args_used<=MAX_OP_ARGS)
		{
			for(int i=0;i<args_used;i++)
			{
				fprintf(stderr," %"PRId64"",args[i]);
			}
		}
		fprintf(stderr,";");
		print_node_ids(0);

		if(endline)
		{
			fprintf(stderr,"\n");
		}
	}
};//end struct Op

//===============================================================================
//===============================================================================
struct PlaylistOp : Op
{
	string playlist_id;

	//=======================================================================
	void init()
	{
		id=PLAYLIST_OP;
		name="playlist";
		playlist_id="";
//		args_used=2;
		regex_match_line="^playlist ['].*[']$";
	}

	//=======================================================================
	PlaylistOp(string input)
	{
		init();

		if(is_match(input))
		{
			string pl_id=input;

			int p = pl_id.find_first_of("'");
			pl_id.erase(0, p+1);

			p = pl_id.find_last_of("'");
			pl_id.erase(p,pl_id.length());

			playlist_id=pl_id;
		}
	}

	//=======================================================================
	void print(uint64_t index)
	{
		fprintf(stderr,"%"PRId64" %d %s '%s'"
			,id_serial
			,id
			,name.c_str()
			,playlist_id.c_str()
			);
	}

	//=======================================================================
	void print()
	{
		print(0);
	}

};//end struct PlaylistOp

//===============================================================================
//===============================================================================
struct FileOp : Op
{
	string uri;

	//=======================================================================
	void init()
	{
		id=FILE_OP;
		name="file";
		uri="";
		args_used=2;
		regex_match_line="^file ['].*['] [0-9]+ [0-9]+$";
	}

	//=======================================================================
	FileOp(string input)
	{
		init();

		if(is_match(input))
		{
			string file_uri=input;
			string digits=input.substr(0,input.length());

			int p = file_uri.find_first_of("'");
			file_uri.erase(0, p+1);

			p = file_uri.find_last_of("'");
			file_uri.erase(p,file_uri.length());

			p = digits.find_last_of("'");
			digits.erase(0,p+1);

			extract_two_digits_(digits,args[0],args[1]);

			uri=file_uri;
		}
	}

	//=======================================================================
	FileOp(string file_uri, uint64_t offset, uint64_t count)
	{
		init();
		uri=file_uri;
		args[0]=offset;
		args[1]=count;
	}

	//=======================================================================
	void undo()
	{
fprintf(stderr,"========UNDO in FILEOP\n");
//remove_last_file();
	}

	//=======================================================================
	void print(uint64_t index)
	{
		fprintf(stderr,"[%"PRId64"] %s '%s' %"PRId64" %"PRId64"\n"
			,index
//			,id_serial
//			,id
			,name.c_str()
			,uri.c_str()
			,args[0]
			,args[1]
			);
	}

	//=======================================================================
	void print()
	{
		print(0);
	}

};//end struct FileOp

//===============================================================================
//===============================================================================
struct InsertOp : Op
{
	//=======================================================================
	void init()
	{
		id=INSERT_OP;
		name="insert";
		args_used=2;
		regex_match_line="^insert [0-9]+ [0-9]+$";
	}

	//=======================================================================
	InsertOp(string input)
	{
		init();

		if(is_match(input))
		{
			extract_two_digits(input,args[0],args[1]);
		}
	}

	//=======================================================================
	InsertOp(uint64_t pos,uint64_t count)
	{
		init();
		args[0]=pos;
		args[1]=count;
	}

	//=======================================================================
	void print(uint64_t index)
	{
		print_common(index,1);
	}
};//end struct InsertOp

//===============================================================================
//===============================================================================
struct InsertFileOp : Op
{
	//=======================================================================
	void init()
	{
		id=INSERT_FILE_OP;
		name="insert_file";
		args_used=2;
		regex_match_line="^insert_file [0-9]+ [0-9]+$";
	}

	//=======================================================================
	InsertFileOp(string input)
	{
		init();

		if(is_match(input))
		{
			extract_two_digits(input,args[0],args[1]);
		}
	}

	//=======================================================================
	InsertFileOp(uint64_t pos,uint64_t file_index)
	{
		init();
		args[0]=pos;
		args[1]=file_index;
	}

	//=======================================================================
	void print(uint64_t index)
	{
		print_common(index,1);
	}
};//end struct InsertFileOp

//===============================================================================
//===============================================================================
struct CutOp : Op
{
	//=======================================================================
	void init()
	{
		id=CUT_OP;
		name="cut";
		args_used=2;
		regex_match_line="^cut [0-9]+ [0-9]+$";
	}

	//=======================================================================
	CutOp()
	{
		init();
	}

	//=======================================================================
	CutOp(string input)
	{
		init();

		if(is_match(input))
		{
			extract_two_digits(input,args[0],args[1]);
		}
	}

	//=======================================================================
	CutOp(uint64_t pos,uint64_t count)
	{
		init();
		args[0]=pos;
		args[1]=count;
	}

	//=======================================================================
	void print(uint64_t index)
	{
		print_common(index,1);
	}
};//end struct CutOp

//===============================================================================
//===============================================================================
struct SilentOp : Op
{
	//=======================================================================
	void init()
	{
		id=SILENT_OP;
		name="silent";
		args_used=2;
		regex_match_line="^silent [0-9]+ [0-9]+$";
	}

	//=======================================================================
	SilentOp()
	{
		init();
	}

	//=======================================================================
	SilentOp(string input)
	{
		init();

		if(is_match(input))
		{
			extract_two_digits(input,args[0],args[1]);
		}
	}

	//=======================================================================
	SilentOp(uint64_t pos,uint64_t count)
	{
		init();
		args[0]=pos;
		args[1]=count;
	}

	//=======================================================================
	void print(uint64_t index)
	{
		print_common(index,1);
	}
};//end struct SilentOp

//===============================================================================
//===============================================================================
struct UndoOp : Op
{
	//=======================================================================
	void init()
	{
		id=UNDO_OP;
		name="undo";
		args_used=0;
		regex_match_line="^undo$";
	}

	//=======================================================================
	UndoOp(string input)
	{
		init();
	}

	//=======================================================================
	UndoOp()
	{
		init();
	}

	//=======================================================================
	void print(uint64_t index)
	{
		print_common(index,1);
	}
};//end struct UndoOp

//===============================================================================
//===============================================================================
struct RedoOp : Op
{
	//=======================================================================
	void init()
	{
		id=REDO_OP;
		name="redo";
		args_used=0;
		regex_match_line="^redo$";
	}

	//=======================================================================
	RedoOp(string input)
	{
		init();
	}

	//=======================================================================
	RedoOp()
	{
		init();
	}

	//=======================================================================
	void print(uint64_t index)
	{
		print_common(index,1);
	}
};//end struct RedoOp

//===============================================================================
//===============================================================================
struct NumericOp : Op
{
	//=======================================================================
	void init()
	{
		id=NUMERIC_OP;
		name="numeric";
		args_used=1;
		regex_match_line="^[0-9]+$";
	}

	//=======================================================================
	NumericOp(string input)
	{
		init();

		if(is_match(input))
		{
			extract_one_digit(input,args[0]);
		}
	}

	//=======================================================================
	NumericOp(uint64_t index)
	{
		init();

		args[0]=index;
	}

	//=======================================================================
	void print(uint64_t index)
	{
		print_common(index,1);
	}
};//end struct RedoOp

//===============================================================================
//===============================================================================
struct Ops
{
	uint64_t op_counter; //for (next) index

	uint64_t can_redo_counter;

	struct Op *ops[MAX_OPS];

	//=======================================================================
	void reset()
	{
		for(int i=0;i<MAX_OPS;i++)
		{
			ops[i]=NULL;
		}
		op_counter=0;
		can_redo_counter=0;
	}

	//=======================================================================
	Ops()
	{
		reset();
	}

	//=======================================================================
	uint64_t add(Op *o)
	{
		ops[op_counter]=o;
		op_counter++;
		return op_counter;
	}

	//=======================================================================
	int is_empty()
	{
		if(op_counter==0)
		{
			return 1;
		}
		return 0;
	}

	//=======================================================================
	uint64_t count()
	{
		return op_counter;
	}

	//=======================================================================
	void undo()
	{
		if(!is_empty())
		{
			last()->undo();
			op_counter--;
			can_redo_counter++;
		}		
	}

	//=======================================================================
	int can_redo()
	{
		return can_redo_counter;
	}

	//=======================================================================
	Op * get_op_at_index(uint64_t index)
	{
		if(index>MAX_OPS-1)
		{
			return NULL;
		}
		return ops[index];
	}
	//=======================================================================
	Op * get_redo_op()
	{
		return ops[op_counter];//-1 + 1];
	}

	//=======================================================================
	void prepare_redo()
	{
		can_redo_counter--;
	}

	//=======================================================================
	void all_redone()
	{
		can_redo_counter=0;
	}

	//=======================================================================
	Op * last()
	{
		return ops[op_counter-1];
	}

	//=======================================================================
	void print()
	{
		if(op_counter==0)
		{
			fprintf(stderr,"//===oplist: empty\n");
			if(can_redo_counter==0)
			{
				return;
			}
		}
		fprintf(stderr,"//===oplist: %"PRId64" items\n//[index] op_literal( arg)*; nid (,nid)*\n"
			,op_counter);
		for(uint64_t i=0;i<op_counter;i++)
		{
			ops[i]->print(i);
		}
		
		if(can_redo_counter>0)
		{
			fprintf(stderr,"--can redo %"PRId64"\n",can_redo_counter);
			for(uint64_t i=0;i<can_redo_counter;i++)
			{
				ops[op_counter+i]->print(op_counter+i);
			}
		}

		fprintf(stderr,"\n");
	}//end print()
};//end struct Ops

#endif 
//EOF
