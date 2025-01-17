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
// kgvcommon.cpp ベクトル要素の参照選択クラス
// =============================================================================
#include <cstdio>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <kgvcommon.h>
#include <kgError.h>
#include <kgMethod.h>
#include <kgConfig.h>

using namespace std;
using namespace kglib;
using namespace kgmod;

// -----------------------------------------------------------------------------
// コンストラクタ(モジュール名，バージョン登録)
// -----------------------------------------------------------------------------
const char * kgVcommon::_ipara[] = {"i","m",""};
const char * kgVcommon::_opara[] = {"o",""};

kgVcommon::kgVcommon(void)
{

	_name    = "kgvcommon";
	_version = "###VERSION###";

	_paralist = "i=,o=,m=,vf=,K=,delim=,-r,-A";
	_paraflg = kgArgs::COMMON|kgArgs::IODIFF|kgArgs::NULL_IN|kgArgs::NULL_OUT;

	#include <help/en/kgvcommonHelp.h>
	_titleL = _title;
	_docL   = _doc;
	#ifdef JPN_FORMAT
		#include <help/jp/kgvcommonHelp.h>
	#endif

}
// -----------------------------------------------------------------------------
// 入出力ファイルオープン
// -----------------------------------------------------------------------------
void kgVcommon::setArgsMain(void)
{
	// 入力ファイルから項目名行を読み込む
	_iFile.read_header();
	_mFile.read_header();

	// vf= 項目引数のセット
	vector< vector<kgstr_t> >  vs = _args.toStringVecVec("vf=","%:",2,true,true);
	_vfField.set(vs, &_iFile,_fldByNum);

	//反転フラグ
	_reverse  = _args.toBool("-r");

	//区切り文字指定
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
	
	// K＝指定:マスタにあるkey
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
// 入出力ファイルオープン
// -----------------------------------------------------------------------------
void kgVcommon::setArgs(void)
{
	// パラメータチェック
	_args.paramcheck(_paralist,_paraflg);

	// 入出力ファイルオープン
	kgstr_t ifile = _args.toString("i=",false);
	kgstr_t mfile = _args.toString("m=",false);
	if(ifile.empty() && mfile.empty()){
		throw kgError("Either i= or m= must be specified.");
	}
	_iFile.open(ifile, _env,_nfn_i);
	_mFile.open(mfile, _env,_nfn_i);
	_oFile.open(_args.toString("o=",false),_env,_nfn_o,_rp);

	setArgsMain();

}

// -----------------------------------------------------------------------------
// 入出力ファイルオープン
// -----------------------------------------------------------------------------
void kgVcommon::setArgs(int inum,int *i_p,int onum ,int *o_p)
{
	int iopencnt = 0;
	int oopencnt = 0;
	try{

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
		iopencnt++;

		if(m_p_t>0){ _mFile.popen(m_p_t, _env,_nfn_i); }
		else if( mfile.empty()){ 
			throw kgError("m= is necessary");
		}
		else       { _mFile.open(mfile, _env,_nfn_i);}
		iopencnt++;

		if(onum == 1 && *o_p > 0){ _oFile.popen(*o_p, _env,_nfn_o,_rp);}
		else{ _oFile.open(_args.toString("o=",true), _env,_nfn_o,_rp);}
		oopencnt++;

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

void kgVcommon::output_n(char *str,bool eol)
{
	*_outstr = '\0'; 
	
	int len=0;

	// itemset項目を" "でトークン分割
	vector<char*> eachItem = splitToken( str ,_delim);

	for(vector<char*>::iterator j=eachItem.begin(); j!=eachItem.end(); j++){
		if((_itmSet.find(*j)!=_itmSet.end())!=_reverse){
			len += ( strlen(_delimstr) + strlen(*j) );
			if(len>=KG_MAX_STR_LEN){ throw kgError("field length exceeded KG_MAX_STR_LEN");}
			if(*_outstr!='\0'){ strcat(_outstr,_delimstr); }
			strcat(_outstr,*j);
		}
	}
	if(_assertNullOUT && *_outstr=='\0'){ _existNullOUT = true;}
	_oFile.writeStr(_outstr,eol);

} 

// -----------------------------------------------------------------------------
// 実行
// -----------------------------------------------------------------------------
int kgVcommon::runMain(void)
{

	// 参照ファイルからitemのhash表を作成する
	while(EOF != _mFile.read() ){
		const char* item = _mFile.getVal(_mkField.num(0));
		if(_assertNullKEY && *item=='\0') { _existNullKEY = true; }
		_itmSet.insert(item);
	}

	//出力項目名出力 追加 or 置換
	if(_add_flg) { _oFile.writeFldName(_iFile,_vfField,true);}
	else				 { _oFile.writeFldName(_vfField, true);}
	int outsize = _iFile.fldSize();
	if(_add_flg) { outsize += _vfField.size(); }	


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

	// 終了処理(メッセージ出力,thread pipe終了通知)
	_iFile.close();
	_mFile.close();
	_oFile.close();
	return 0;

}
// -----------------------------------------------------------------------------
// 実行 
// -----------------------------------------------------------------------------
int kgVcommon::run(void)
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
    ((kgVcommon*)arg)->runErrEnd();
}

int kgVcommon::run(int inum,int *i_p,int onum, int* o_p,string &msg)
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
