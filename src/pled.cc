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

/*
pled - playlist editor

this is experimental code. it is whether complete nor documented or fully tested 
or otherwise fit for anything other than playing around at this time
*/

#include "eventual/timeline.h"

using namespace std;

//variable containing current line
string LINE="";

//parser status
int START_FOUND=0;
int END_FOUND=0;
int HAVE_FILE=0;

int verbose=1;

Timeline *tl=NULL;

//=============================================================
//pl::
int find_pl_start()
{
	if(! re(LINE,"^pl[:][:]$"))
	{
		START_FOUND=1;
		if(verbose)
		{
			fprintf(stderr,"START FOUND\n");
		}
		return 0;
	}
	return 1;
}

//=============================================================
//::
int find_pl_end()
{
	if(! re(LINE,"^[:][:]$"))
	{
		END_FOUND=1;
		if(verbose)
		{
			fprintf(stderr,"END FOUND\n");
		}
		return 0;
	}
	return 1;
}

//=============================================================
int handle_empty_line()
{
	if(! re(LINE,"^$"))
	{
		return 0;
	}
	return 1;
}

//=============================================================
int handle_comment()
{
	//comment line
	if (! re(LINE,"^//"))
	{
		return 0;
	}
	else if (! re(LINE,"^#"))
	{
		return 0;
	}
	return 1;
}

//=============================================================
//help
int handle_help()
{
	if(! re(LINE,"^help$"))
	{
		fprintf(stderr,"=help (dummy)\n");
///
//print commands here
		return 0;
	}
	return 1;
}

//=============================================================
//verbose on/off
//not used consistently yet
int handle_verbose()
{
	if(! re(LINE,"^verbose on$"))
	{
		verbose=1;
		fprintf(stderr,"=verbose: on\n");
		return 0;
	}
	else if(! re(LINE,"^verbose off$"))
	{
		verbose=0;
//		fprintf(stderr,"=verbose: off\n");
		return 0;
	}
	return 1;
}

PlaylistOp *playlist_op=new PlaylistOp("");
//=============================================================
//playlist 'alphanumeric id'
int handle_new_playlist()
{
	if(playlist_op->is_match(LINE))
	{
		HAVE_FILE=1;	
		PlaylistOp *op=new PlaylistOp(LINE);
///
		tl=new Timeline(op->playlist_id);

		return 0;
	}
	else
	{
		return 1;
	}
}

FileOp *file_op=new FileOp("",0,0);
//=============================================================
//file 'file uri' <frame_offset> <frame_count>
int handle_file()
{
	if(file_op->is_match(LINE))
	{
		FileOp *op=new FileOp(LINE);
		tl->exec(op);
		return 0;
	}
	else
	{
		return 1;
	}
}

InsertOp *insert_op=new InsertOp(0,0);
//=============================================================
//insert <pos> <count>
int handle_insert()
{
	if(insert_op->is_match(LINE))
	{
		tl->exec(new InsertOp(LINE));
		return 0;
	}
	else
	{
		return 1;
	}
}

InsertFileOp *insert_file_op=new InsertFileOp(0,0);
//=============================================================
//insert_file <pos> <file index>
int handle_insert_file()
{
	if(insert_file_op->is_match(LINE))
	{
		tl->exec(new InsertFileOp(LINE));
		return 0;
	}
	else
	{
		return 1;
	}
}

CutOp *cut_op=new CutOp(0,0);
//=============================================================
//cut <pos> <count>
int handle_cut()
{
	if(cut_op->is_match(LINE))
	{
		tl->exec(new CutOp(LINE));
		return 0;
	}
	else
	{
		return 1;
	}
}

SilentOp *silent_op=new SilentOp(0,0);
//=============================================================
//silent <pos> <count>
int handle_silent()
{
	if(silent_op->is_match(LINE))
	{
		tl->exec(new SilentOp(LINE));
		return 0;
	}
	else
	{
		return 1;
	}
}

UndoOp *undo_op=new UndoOp();
//=============================================================
//undo
int handle_undo()
{
	if(undo_op->is_match(LINE))
	{
		tl->exec(new UndoOp(LINE));
		return 0;
	}
	else
	{
		return 1;
	}
}

RedoOp *redo_op=new RedoOp();
//=============================================================
//redo
int handle_redo()
{
	if(redo_op->is_match(LINE))
	{
		tl->exec(new RedoOp(LINE));
		return 0;
	}
	else
	{
		return 1;
	}
}

NumericOp *numeric_op=new NumericOp(0);
//=============================================================
//repeat (once) command from oplist
//<index>
int handle_numeric()
{
	if(numeric_op->is_match(LINE))
	{
		tl->exec(new NumericOp(LINE));
		return 0;
	}
	else
	{
		return 1;
	}
}

//=============================================================
int handle_test()
{
	if(! re(LINE,"^test [0-9]+ [0-9]+$"))
	{
		fprintf(stderr,"test op: ");

		uint64_t count=0;
		uint64_t pos=0;

		extract_two_digits(LINE,count,pos);
///

		return 0;
	}
	return 1;
}

//=============================================================
//print
int handle_print()
{
	if(! re(LINE,"^print$"))
	{
		tl->eval(1);
		return 0;
	}
	return 1;
}

/*
=============================================================
MAIN LOOP
=============================================================
*/

int main()
{
	//string line;
	while (getline(cin, LINE))
	{
		trim_leading(LINE);
		trim_trailing(LINE);
		remove_ctrl_m(LINE);

		if(! handle_verbose()){continue;}

		if(verbose)
		{
			fprintf(stderr,"input-%s-\n",LINE.c_str());
		}

		if(! handle_help()){continue;}

		if(! handle_empty_line()){continue;}
		if(! handle_comment()){continue;}

		if(!START_FOUND)
		{
			if(! find_pl_start()){continue;}
		}
		else
		{
			if(! find_pl_end()){break;}
			if(! handle_new_playlist()){continue;}

			if(HAVE_FILE)
			{
				if(! handle_file()){continue;}
				if(! handle_insert_file()){continue;}

				if(! handle_cut()){continue;}
				if(! handle_silent()){continue;}
				if(! handle_insert()){continue;}
				if(! handle_undo()){continue;}
				if(! handle_redo()){continue;}
				if(! handle_numeric()){continue;}
				if(! handle_print()){continue;}

				if(! handle_test()){continue;}
			}
		}

		if(verbose)
		{
			fprintf(stderr,"/!\\ command unknown or not allowed in this context\n");
		}

	} //end while read line from file

	if(!START_FOUND)
	{
		fprintf(stderr,"/!\\ start of playlist not found. looking for '::pl'\n");
		return 1;
	}
	else if(!END_FOUND)
	{
		fprintf(stderr,"/!\\ unclosed playlist. looking for '::'\n");
		return 1;
	}

	if(!HAVE_FILE)
	{
		fprintf(stderr,"/!\\ empty playlist. looking for 'playlist ...'\n");
	}

	return 0;

}//end main

/*
g++ -o pled pled.cc && cat pled.cc | ./pled

//first playlist found in file will be used

//empty playlist
//pl::
//::

//verbose off
pl::
//comment
	#also a comment

playlist 'xyz.42'
file 'a.wav' 0 1000
insert_file 0 1
cut 100 10
print
::
*/
