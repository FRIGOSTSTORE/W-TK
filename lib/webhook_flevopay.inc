<?php
require_once __DIR__.'/tracker.inc';require_once __DIR__.'/transaction_store.inc';require_once __DIR__.'/gateway_stats.inc';
header('Content-Type: application/json; charset=utf-8');if($_SERVER['REQUEST_METHOD']!=='POST'){http_response_code(405);exit;}
$p=json_decode(file_get_contents('php://input'),true)??[];$tx=preg_replace('/[^a-zA-Z0-9]/','',(string)($p['external_id']??$p['id']??''));$st=strtolower((string)($p['status']??''));
if($tx===''||$st!=='approved'){http_response_code(200);echo json_encode(['ok'=>true,'ignored'=>true]);exit;}
try{
 $s=new TransactionStore();$t=$s->carregar($tx);
 if(empty($t)||($t['gateway']??'')!=='flevopay'){echo json_encode(['ok'=>true,'ignored'=>true]);exit;}
 if(($t['status']??'')==='paid'){echo json_encode(['ok'=>true,'already_processed'=>true]);exit;}
 if(!$s->adquirirLock($tx)){echo json_encode(['ok'=>true,'locked'=>true]);exit;}
 try{
  $d=array_merge($t,['txid'=>$tx,'status'=>'paid','paidAt'=>$p['timestamp']??date('Y-m-d H:i:s'),
  'valor'=>isset($p['amount'])?number_format(((int)$p['amount'])/100,2,'.',''):($t['valor']??'0.00'),
  'endToEndId'=>$p['transaction_id']??null]);
  (new Tracker())->purchase($d);$s->salvar($tx,$d);(new GatewayStats())->increment('flevopay','paid');
 }finally{$s->liberarLock($tx);}
 echo json_encode(['ok'=>true]);
}catch(Throwable $e){http_response_code(500);echo json_encode(['ok'=>false,'erro'=>$e->getMessage()]);}
