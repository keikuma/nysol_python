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
// kgjoin.cpp 参照ファイル項目の結合
// =============================================================================
#include <cstdio>
#include <sstream>
#include <vector>
#include <kgunicat.h>
#include <kgError.h>
#include <kgConfig.h>

using namespace std;
using namespace kglib;
using namespace kgmod;

// -----------------------------------------------------------------------------
// コンストラクタ(モジュール名，バージョン登録)
// -----------------------------------------------------------------------------
const char * kgUnicat::_ipara[] = {"i","m",""};
const char * kgUnicat::_opara[] = {"o",""};

kgUnicat::kgUnicat(void)
{
	_name    = "kgunicat";
	_version = "###VERSION###";

	_paralist = "i=,o=,m=,k=,K=,-q,-nouniq";
	_paraflg = kgArgs::ALLPARAM;


	#include <help/en/kgunicatHelp.h>
	_titleL = _title;
	_docL   = _doc;
	#ifdef JPN_FORMAT
		#include <help/jp/kgunicatHelp.h>
	#endif
	
}

void kgUnicat::setArgsMain(void){

	// k= 項目引数のセット
	vector<kgstr_t> vs_k = _args.toStringVector("k=",true);

	// K= 項目引数のセット
	// K=の指定がなければk=の値をセットする
	// k=とK=の数があっているかチェック
	vector<kgstr_t> vs_K = _args.toStringVector("K=",false);
	if(vs_K.empty()){ vs_K = vs_k; }
	if(vs_K.size()!=vs_k.size()){
		ostringstream ss;
		ss << "unmatched key size (" << vs_k.size() << " fields on k=, but " << vs_K.size() << " fields on K=)";
		throw kgError(ss.str());
	}

	_iFile.read_header();
	_mFile.read_header();


	_nouniq = _args.toBool("-nouniq");
	
	bool seqflg = _args.toBool("-q");
	if(_nfn_i) { seqflg = true; }

	if( !seqflg ){
		vector<kgCSVfld*> csv_p;  
		vector< vector<kgstr_t> > fld_ary;  
		csv_p.push_back(&_iFile);
		csv_p.push_back(&_mFile);
		fld_ary.push_back(vs_k);
		fld_ary.push_back(vs_K);
		sortingRun(csv_p,fld_ary);
	}
	
	_kField.set(vs_k, &_iFile,_fldByNum);
	_KField.set(vs_K, &_mFile,_fldByNum);


}

// -----------------------------------------------------------------------------
// パラメータセット＆入出力ファイルオープン
// -----------------------------------------------------------------------------
void kgUnicat::setArgs(void)
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
  _oFile.open(_args.toString("o=",false), _env,_nfn_o,_rp);

	setArgsMain();
}

void kgUnicat::setArgs(int inum,int *i_p,int onum ,int *o_p)
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


int kgUnicat::runMain()
{
	// 項目名出力
  _oFile.writeFldName(_iFile);

	// keyサイズとデータセット
	int ksize = _kField.size();
	vector<int> kField =_kField.getNum();
	vector<int> KField =_KField.getNum();

	if(ksize == 0){
		ksize = _iFile.fldSize();
		kField.clear();
		KField.clear();
		for(int i = 0 ; i<ksize;i++){
			kField.push_back(i);
			KField.push_back(i);
		}
	}
	_iFile.setKey(kField);
	_mFile.setKey(KField);


	//比較結果用フラグ&出力チェックフラグ
	int cmpflg=0;
	bool wflg=false;
	bool traEnd=false;
	bool mstEnd=false;
	bool begin=true;

	// データ出力
	while(true){
		// traの読み込み
		if(cmpflg<=0){
			while(true){
				if( _iFile.read() == EOF){ traEnd=true;}
				if( _iFile.begin() ){ continue;}
				if(_nouniq){ break;}
				if( _iFile.keybreak() ){ break;}
			}
		}

		// mstの読み込み
		if(cmpflg> 0 || begin){
			while(true){
				if( _mFile.read() == EOF){ mstEnd=true;}
				if( _mFile.begin() ){ continue;}
				if(_nouniq){ 
					wflg=false;
					begin=false;
					break;
				}

				if( _mFile.keybreak() ){ 
					wflg=false;
					begin=false;
					break;
				}
			}
		}

		// キーの比較 (tra - mstの演算結果)
		if(traEnd){
			if(mstEnd){ break;}
			else      { cmpflg=1;}
		}else if(mstEnd){
			cmpflg=-1;
		} else {
			cmpflg=0;
			for(int i=0;i<ksize;i++){
				if(_assertNullKEY) { 
					if( *(_iFile.getOldVal(kField[i]))=='\0' || *(_mFile.getOldVal(KField[i]))=='\0'){
						_existNullKEY = true;
					}
				}
				cmpflg = strcmp( _iFile.getOldVal(kField[i]), _mFile.getOldVal(KField[i]) );
				if(cmpflg!=0) break;
			}
		}		
		if(cmpflg==0){
			// 一致
			_oFile.writeFld(_iFile.fldSize(),_iFile.getOldFld());
			if(_nouniq&& !wflg){
				_oFile.writeFld(_mFile.fldSize(),_mFile.getOldFld());
			}
			wflg=true;

		}else if( cmpflg>0 && !wflg ){
			// mst書き出す
			_oFile.writeFld(_mFile.fldSize(),_mFile.getOldFld());
			wflg=true;
		}else if( cmpflg<0 ){
			_oFile.writeFld(_iFile.fldSize(),_iFile.getOldFld());
		}
	}
	//ソートスレッドを終了させて、終了確認
	//for(size_t i=0 ;i<_th_st.size();i++){ pthread_cancel(_th_st[i]->native_handle());	}
	//for(size_t i=0 ;i<_th_st.size();i++){ pthread_join(_th_st[i]->native_handle(),NULL);}

	// 終了処理
	_iFile.close();
	_mFile.close();
	_oFile.close();
	th_cancel();
	return 0;
}


// -----------------------------------------------------------------------------
// 実行
// -----------------------------------------------------------------------------
int kgUnicat::run(void) 
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
    ((kgUnicat*)arg)->runErrEnd();
}

int kgUnicat::run(int inum,int *i_p,int onum, int* o_p,string &msg)
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




