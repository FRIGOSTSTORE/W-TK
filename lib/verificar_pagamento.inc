<?php
error_reporting(0);ini_set('display_errors','0');header('Content-Type: application/json; charset=utf-8');
require_once __DIR__.'/gateway_router.inc';require_once __DIR__.'/tracker.inc';require_once __DIR__.'/transaction_store.inc';require_once __DIR__.'/gateway_stats.inc';
$tx=preg_replace('/[^a-zA-Z0-9]/','',$_GET['txid']??'');if($tx===''){http_response_code(400);echo json_encode(['erro'=>'txid invalido.']);exit;}
try{$s=new TransactionStore();$t=$s->carregar($tx);}catch(Throwable $e){$t=[];}
if(($t['status']??'')==='paid'){echo json_encode(['txid'=>$tx,'status'=>'paid','gateway'=>$t['gateway']??null]);exit;}
$gw=$t['gateway']??(stripos($tx,'AJF')===0?'flevopay':'basspago');
try{
 $c=(new GatewayRouter())->query($gw,$tx);$st=strtolower((string)($c['status']??''));
 $paid=$gw==='flevopay'?($st==='approved'):($gw==='winnerpay'?in_array($st,['paid','completed'],true):in_array(strtoupper($st),['CONCLUIDA','CONCLUIDO','PAGA','PAGO','PAID','COMPLETED'],true));
 if(!$paid&&$gw==='basspago'&&!empty($c['pix']))$paid=true;
 if(!$paid){echo json_encode(['txid'=>$tx,'status'=>'waiting_paid','gateway'=>$gw,'statusPsp'=>$st]);exit;}
 $d=array_merge($t,['txid'=>$tx,'status'=>'paid','paidAt'=>date('Y-m-d H:i:s'),
 'valor'=>$c['amount_in_reais']??$c['valor']['original']??$t['valor']??'0.00']);
 if(!$s->adquirirLock($tx)){
  $fresh=$s->carregar($tx);
  echo json_encode(['txid'=>$tx,'status'=>$fresh['status']??'paid','gateway'=>$gw]); exit;
 }
 try{
  $fresh=$s->carregar($tx);
  if(($fresh['status']??'')==='paid'){echo json_encode(['txid'=>$tx,'status'=>'paid','gateway'=>$gw]);exit;}
  (new Tracker())->purchase($d);
  $s->salvar($tx,$d);
  (new GatewayStats())->increment($gw,'paid');
 }finally{$s->liberarLock($tx);}
 echo json_encode(['txid'=>$tx,'status'=>'paid','gateway'=>$gw,'paidAt'=>$d['paidAt']]);
}catch(Throwable $e){echo json_encode(['txid'=>$tx,'status'=>$t['status']??'waiting_paid','gateway'=>$gw]);}
