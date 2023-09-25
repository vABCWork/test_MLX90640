
#include <machine.h>

//    　例外事象の処理プログラム
// 　未定義命令例外、特権命令例外、アクセス例外、浮動小数点例外、ノンマスカブルの処理プログラム
//      brk() : 無条件トラップ (割り込みベクタテーブルのベクタ番号0のアドレスに分岐) 


// 特権命令例外 (ユーザモードで特権命令の実行を検出した場合に発生)
#pragma interrupt (Excep_SuperVisorInst)
void Excep_SuperVisorInst(void){
	brk();
}

// アクセス例外 (CPU からのメモリアクセスによるエラーが検出された場合に発生)
#pragma interrupt (Excep_AccessInst)
void Excep_AccessInst(void){
	brk();
}

//  未定義命令例外 (未定義命令( 実装されていない命令) の実行を検出した場合に発生)
#pragma interrupt (Excep_UndefinedInst)
void Excep_UndefinedInst(void){
	brk();
}

// 浮動小数点例外
// オーバフロー、アンダフロー、精度異常、ゼロ除算、無効演算)の他、非実装処理を検出した場合に発生。
// FPSW のEX、EU、EZ、EO、EV ビットが“0” のとき、例外処理が禁止されます。
// (リセット後の値は全て0のため、例外処理禁止)
//
#pragma interrupt (Excep_FloatingPoint)
void Excep_FloatingPoint(void){
	brk();
}

// ノンマスカブル割り込み NMI (CPU にノンマスカブル割り込み信号を入力することによって発生)
// NMI端子(P35), 独立ウオッチドックタイマ(IWDT)によるノンマスカブル割り込み
#pragma interrupt (NonMaskableInterrupt)
void NonMaskableInterrupt(void){
	brk();
}

// Dummy
#pragma interrupt (Dummy)
void Dummy(void){
	brk();
}
