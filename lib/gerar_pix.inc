<?php
error_reporting(E_ALL & ~E_WARNING & ~E_NOTICE & ~E_DEPRECATED); ini_set('display_errors','0');
header('Content-Type: application/json; charset=utf-8');
require_once __DIR__.'/gateway_router.inc'; require_once __DIR__.'/tracker.inc';
require_once __DIR__.'/transaction_store.inc'; require_once __DIR__.'/gateway_stats.inc';
if($_SERVER['REQUEST_METHOD']!=='POST'){http_response_code(405);echo json_encode(['erro'=>'Metodo nao permitido.']);exit;}
$i=json_decode(file_get_contents('php://input'),true)??[];
try{
 $r=(new GatewayRouter())->create($i);$o=$r['order'];$tx=$r['txid'];$gw=$r['gateway'];
 $d=['txid'=>$tx,'gateway'=>$gw,'valor'=>$r['valor'],'descricao'=>$o['descricao'],'createdAt'=>date('Y-m-d H:i:s'),
 'ua'=>$_SERVER['HTTP_USER_AGENT']??'','url'=>$o['url'],'email'=>$o['email'],'phone'=>$o['phone'],'document'=>$o['document'],
 'nome'=>$o['nome'],'fbc'=>$o['fbc'],'fbp'=>$o['fbp'],'utm_source'=>$o['utm_source'],'utm_medium'=>$o['utm_medium'],
 'utm_campaign'=>$o['utm_campaign'],'utm_content'=>$o['utm_content'],'utm_term'=>$o['utm_term'],'src'=>$o['src'],'sck'=>$o['sck'],'ttclid'=>$o['ttclid'],
 'status'=>'waiting_paid'];
 (new TransactionStore())->salvar($tx,$d); try{(new GatewayStats())->increment($gw,'generated');}catch(Throwable $e){}
 (new Tracker())->initiateCheckout($d);
 echo json_encode(['txid'=>$tx,'gateway'=>$gw,'status'=>$r['status'],'valor'=>$r['valor'],'expiracao'=>$r['expiracao'],
 'pixCopiaECola'=>$r['pixCopiaECola'],'flevopay_transaction_id'=>$r['flevopay_transaction_id']??null,'winnerpay_transaction_id'=>$r['winnerpay_transaction_id']??null],JSON_UNESCAPED_UNICODE);
}catch(Throwable $e){http_response_code(($e->getCode()>=400&&$e->getCode()<600)?$e->getCode():500);echo json_encode(['erro'=>$e->getMessage()],JSON_UNESCAPED_UNICODE);}
