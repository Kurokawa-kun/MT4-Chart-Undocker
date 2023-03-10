//+------------------------------------------------------------------+
//|                                                       Undock.mq4 |
//|                                         Copyright 2022, Kurokawa |
//|                                   https://twitter.com/ImKurokawa |
//+------------------------------------------------------------------+
#property copyright "Copyright 2022, Kurokawa"
#property link      "https://twitter.com/ImKurokawa"
#property version   "1.01"
#property description   "チャート画面をMT4本体から切り離します。"
#property icon "Undock.ico"
#property strict

#import "UndockLibrary.dll"
   int Undock(string WindowTitle, int Handle);
#import

void OnStart()
{
   if (!IsDllsAllowed())
   {
      Print("'DLLの使用を許可する'にチェックを入れてください。");
      return;
   }
   int wh = WindowHandle(Symbol(), PERIOD_CURRENT);   //  チャートウィンドウのウィンドウハンドラを取得
   string tf = NULL; //  表示しているチャートの時間足を文字列型で取得する
   switch (Period())
   {
      case PERIOD_M1:
      {
         tf = "M1";
         break;
      }
      case PERIOD_M5:
      {
         tf = "M5";
         break;
      }
      case PERIOD_M15:
      {
         tf = "M15";
         break;
      }
      case PERIOD_M30:
      {
         tf = "M30";
         break;
      }
      case PERIOD_H1:
      {
         tf = "H1";
         break;
      }
      case PERIOD_H4:
      {
         tf = "H4";
         break;
      }
      case PERIOD_D1:
      {
         tf = "Daily";
         break;
      }
      case PERIOD_W1:
      {
         tf = "Weekly";
         break;
      }
      case PERIOD_MN1:
      {
         tf = "Monthly";
         break;
      }
      default:
      {
         tf = "";
         break;
      }
   }
   //  DLLを呼び出す
   int rt = Undock(StringFormat("%s,%s", Symbol(), tf), wh);
   return;
}
