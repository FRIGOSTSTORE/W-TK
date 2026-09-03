<?php
require_once __DIR__.'/var.inc';
require_once __DIR__.'/tracker.inc';
require_once __DIR__.'/transaction_store.inc';
require_once __DIR__.'/gateway_stats.inc';
header('Content-Type: application/json; charset=utf-8');
if($_SERVER['REQUEST_METHOD']!=='POST'){http_response_code(405);exit;}
$raw=file_get_contents('php://input');
$p=json_decode($raw,true)??[];
$signature=$_SERVER['HTTP_X_WINNERPAY_SIGNATURE']??'';
global $WINNERPAY_CLIENT_SECRET;
if($signature!==''){
 $expected='sha256='.hash_hmac('sha256',$raw,(string)$WINNERPAY_CLIENT_SECRET);
 if(!hash_equals($expected,$signature)){http_response_code(401);echo json_encode(['ok'=>false,'erro'=>'Assinatura inválida.']);exit;}
}
$tx=preg_replace('/[^a-zA-Z0-9]/','',(string)($p['external_id']??$p['transaction_id']??''));
$st=strtolower((string)($p['status']??''));
if($tx===''||!in_array($st,['paid','completed'],true)){http_response_code(200);echo json_encode(['ok'=>true,'ignored'=>true]);exit;}
try{
 $s=new TransactionStore();$t=$s->carregar($tx);
 if(empty($t)||($t['gateway']??'')!=='winnerpay'){echo json_encode(['ok'=>true,'ignored'=>true]);exit;}
 if(($t['status']??'')==='paid'){echo json_encode(['ok'=>true,'already_processed'=>true]);exit;}
 if(!$s->adquirirLock($tx)){echo json_encode(['ok'=>true,'locked'=>true]);exit;}
 try{
  $d=array_merge($t,['txid'=>$tx,'status'=>'paid','paidAt'=>$p['updated_at']??date('Y-m-d H:i:s'),'valor'=>isset($p['amount'])?number_format((float)$p['amount'],2,'.',''):($t['valor']??'0.00'),'endToEndId'=>$p['transaction_id']??null]);
  (new Tracker())->purchase($d);$s->salvar($tx,$d);(new GatewayStats())->increment('winnerpay','paid');
 }finally{$s->liberarLock($tx);}
 echo json_encode(['ok'=>true]);
}catch(Throwable $e){http_response_code(500);echo json_encode(['ok'=>false,'erro'=>$e->getMessage()]);}
