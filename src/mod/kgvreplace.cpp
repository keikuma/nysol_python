/* ////////// LICENSE INFO ////////////////////

 * Copyright (C) 2013 by NYSOL CORPORATION
 *
 * Unless you have received this program directly from NYSOL pursuant
 * to the terms of a commercial license agreement with NYSOL, then
 * this program is licensed to you under the terms of the GNU Affero General
 * Public License (AGPL) as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THOSE OF 
 * NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Please refer to the AGPL (http://www.gnu.org/licenses/agpl-3.0.txt)
 * for more details.

 ////////// LICENSE INFO ////////////////////*/
// =============================================================================
// kgvreplace.cpp ベクトル要素の参照置換クラス
// =============================================================================
#include <cstdio>
#include <sstream>
#include <vector>
#include <map>
#include <kgvreplace.h>
#include <kgError.h>
#include <kgMethod.h>
#include <kgConfig.h>

using namespace std;
using namespace kglib;
using namespace kgmod;

// -----------------------------------------------------------------------------
// コンストラクタ(モジュール名，バージョン登録)
// -----------------------------------------------------------------------------
const char * kgVreplace::_ipara[] = {"i","m",""};
const char * kgVreplace::_opara[] = {"o",""};

kgVreplace::kgVreplace(void)
{
	_name    = "kgvreplace";
	_version = "###VERSION###";
	_paralist = "i=,o=,m=,n=,vf=,f=,K=,delim=,-A";
	_paraflg = kgArgs::COMMON|kgArgs::IODIFF|kgArgs::NULL_IN|kgArgs::NULL_OUT;

	#include <help/en/kgvreplaceHelp.h>
	_titleL = _title;
	_docL   = _doc;
	#ifdef JPN_FORMAT
		#include <help/jp/kgvreplaceHelp.h>
	#endif

}

void kgVreplace::setArgsMain(void)
{
	_iFile.read_header();
	_mFile.read_header();

	// vf= 項目引数のセット
	vector < vector<kgstr_t> >  vs = _args.toStringVecVec("vf=","%:",2,true);
	_vfField.set(vs, &_iFile,_fldByNum);

	// f= 項目引数のセット
	vector<kgstr_t>  vsf = _args.toStringVector("f=",true);
	if(vsf.size()!=1){ throw kgError("f= takes just one item "); }
	_fField.set(vsf, &_mFile,_fldByNum);

	// n= 項目引数のセット
	_nullVal = _args.toString("n=",false);

	// delim= 項目引数のセット
	kgstr_t s_d = _args.toString("delim=",false);
	if(s_d.empty()){	
		_delim=' ';
	}else if(s_d.size()!=1){
		ostringstream ss;
		ss << "delim= takes 1 byte charactor (" << s_d << ")";
		throw kgError(ss.str());
	}else{
		_delim=*(s_d.c_str());
	}
	
	// K= 項目引数のセット
	vector<kgstr_t>  vs2 = _args.toStringVector("K=",true);
	if(vs2.size()!=1){ throw kgError("K= takes just one item "); }
	_mkField.set(vs2, &_mFile,_fldByNum);	

	// -A（追加）フラグセット
	_add_flg 		= _args.toBool("-A");

	//文字列生成用領域
	_delimstr[0] =_delim;
	_delimstr[1] ='\0';

}
// -----------------------------------------------------------------------------
// パラメータセット＆入出力ファイルオープン
// -----------------------------------------------------------------------------
void kgVreplace::setArgs(void)
{
	// パラメータチェック
	_args.paramcheck(_paralist,_paraflg);

	// 入出力ファイルオープン
	_iFile.open(_args.toString("i=",false),_env,_nfn_i);
	_oFile.open(_args.toString("o=",false),_env,_nfn_o,_rp);
	_mFile.open(_args.toString("m=",true),_env,_nfn_i);

	setArgsMain();

}
// -----------------------------------------------------------------------------
// パラメータセット＆入出力ファイルオープン
// -----------------------------------------------------------------------------
void kgVreplace::setArgs(int inum,int *i_p,int onum ,int *o_p)
{
	int iopencnt = 0;
	int oopencnt = 0;

	try{
		// パラメータチェック
		_args.paramcheck(_paralist,_paraflg);

		if(inum>2 && onum>1){ throw kgError("no match IO"); }

		// 入出力ファイルオープン
		kgstr_t ifile = _args.toString("i=",false);
		kgstr_t mfile = _args.toString("m=",false);

		int i_p_t = -1;
		int m_p_t = -1;
		if(inum>0){ i_p_t = *i_p;     }
		if(inum>1){ m_p_t = *(i_p+1); }

		if((ifile.empty()&&i_p_t<=0) && ( mfile.empty()&&m_p_t<=0)){
			throw kgError("Either i= or m= must be specified.");
		}

		// 入出力ファイルオープン
		if(i_p_t>0){ _iFile.popen(i_p_t, _env,_nfn_i); }
		else if( ifile.empty()){ 
			throw kgError("i= is necessary");
		}
		else       { _iFile.open(ifile, _env,_nfn_i);}

		if(m_p_t>0){ _mFile.popen(m_p_t, _env,_nfn_i); }
		else if( mfile.empty()){ 
			throw kgError("m= is necessary");
		}
		else       { _mFile.open(mfile, _env,_nfn_i);}

		if(onum == 1 && *o_p > 0){ _oFile.popen(*o_p, _env,_nfn_o,_rp);}
		else{ _oFile.open(_args.toString("o=",false), _env,_nfn_o,_rp);}

		setArgsMain();

	}catch(...){
		for(int i=iopencnt; i<inum ;i++){
			if(*(i_p+i)>0){ ::close(*(i_p+i)); }
		}
		for(int i=oopencnt; i<onum ;i++){
			if(*(o_p+i)>0){ ::close(*(o_p+i)); }
		}
		throw;
	}
		
	
}

void kgVreplace::output_n(char *str,bool eol)
{
	*_outstr = '\0'; 
	
	int len=0;

	vector<char*> eachItem = splitToken( str ,_delim);
	for(vector<char*>::iterator j=eachItem.begin(); j!=eachItem.end(); j++){
		if(_itmSet.find(*j)!=_itmSet.end()){
			len += ( strlen(_delimstr)+strlen(_itmSet[*j].c_str()) );
			if(len>=KG_MAX_STR_LEN){ throw kgError("field length exceeded KG_MAX_STR_LEN");}
			if(*_outstr!='\0'){ strcat(_outstr,_delimstr);}
			strcat(_outstr,_itmSet[*j].c_str());
		}
		else{
			if(!_nullVal.empty()){
				len += ( strlen(_delimstr)+strlen(_nullVal.c_str()) );
				if(len>=KG_MAX_STR_LEN){ throw kgError("field length exceeded KG_MAX_STR_LEN");}
				if(*_outstr!='\0'){ strcat(_outstr,_delimstr);}
				strcat(_outstr,_nullVal.c_str());
			}
			else{
				len += ( strlen(_delimstr)+strlen(*j) );
				if(len>=KG_MAX_STR_LEN){ throw kgError("field length exceeded KG_MAX_STR_LEN");}
				if(*_outstr!='\0'){ strcat(_outstr,_delimstr);}
				strcat(_outstr,*j);
			}
		}
	}
	if(_assertNullOUT && *_outstr=='\0'){ _existNullOUT = true;}
	_oFile.writeStr(_outstr,eol);

} 

// -----------------------------------------------------------------------------
// 実行
// -----------------------------------------------------------------------------
int kgVreplace::runMain(void)
{

	// 参照ファイルからitemのhash表を作成する
	while(EOF != _mFile.read() ){
		const char* item = _mFile.getVal(_mkField.num(0));
		const char* taxo = _mFile.getVal(_fField.num(0));
		if(_assertNullKEY && *item=='\0') { _existNullKEY = true; }
		_itmSet[item]=taxo;
	}

	//出力項目名出力 追加 or 置換
	if(_add_flg) { _oFile.writeFldName(_iFile,_vfField,true);}
	else				 { _oFile.writeFldName(_vfField, true);}
	int outsize = _iFile.fldSize();
	if(_add_flg) { outsize += _vfField.size(); }	

	// データ出力
	while(EOF != _iFile.read() ){
		int outcnt=0;
		for(size_t i=0; i<_iFile.fldSize(); i++){

			outcnt++;
			char* str=_iFile.getVal(i);
			if(_add_flg||_vfField.flg(i)==-1){
				_oFile.writeStr(str,outcnt==outsize);
			}
			else{
				if(_assertNullIN && *str=='\0' ) { _existNullIN  = true;}
				output_n(str,outcnt==outsize);
			}
		}
		if(_add_flg){
			for(kgstr_t::size_type i=0 ; i< _vfField.size() ;i++){
				outcnt++;
				if(_assertNullIN && *_iFile.getVal(_vfField.num(i))=='\0') { _existNullIN  = true;}
				output_n(_iFile.getVal(_vfField.num(i)),outcnt==outsize);
			}
		}		
	}
	_iFile.close();
	_mFile.close();
	_oFile.close();

	return 0;

}
// -----------------------------------------------------------------------------
// 実行 
// -----------------------------------------------------------------------------
int kgVreplace::run(void)
{
	try {

		setArgs();
		int sts = runMain();
		successEnd();
		return sts;

	}catch(kgOPipeBreakError& err){

		runErrEnd();
		successEnd();
		return 0;

	}catch(kgError& err){

		runErrEnd();
		errorEnd(err);
	}catch (const exception& e) {

		runErrEnd();
		kgError err(e.what());
		errorEnd(err);
	}catch(char * er){

		runErrEnd();
		kgError err(er);
		errorEnd(err);
	}catch(...){

		runErrEnd();
		kgError err("unknown error" );
		errorEnd(err);
	}
	return 1;

}

///* thraad cancel action
static void cleanup_handler(void *arg)
{
    ((kgVreplace*)arg)->runErrEnd();
}

int kgVreplace::run(int inum,int *i_p,int onum, int* o_p,string &msg)
{
	int sts=1;
	pthread_cleanup_push(&cleanup_handler, this);	

	try {

		setArgs(inum, i_p, onum,o_p);
		sts = runMain();
		msg.append(successEndMsg());

	}catch(kgOPipeBreakError& err){

		runErrEnd();
		msg.append(successEndMsg());
		sts = 0;

	}catch(kgError& err){

		runErrEnd();
		msg.append(errorEndMsg(err));

	}catch (const exception& e) {

		runErrEnd();
		kgError err(e.what());
		msg.append(errorEndMsg(err));

	}catch(char * er){

		runErrEnd();
		kgError err(er);
		msg.append(errorEndMsg(err));

	}
	KG_ABI_CATCH
	catch(...){

		runErrEnd();
		kgError err("unknown error" );
		msg.append(errorEndMsg(err));

	}

  pthread_cleanup_pop(0);
	return sts;

}
