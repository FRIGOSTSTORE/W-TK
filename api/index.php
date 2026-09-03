<?php
declare(strict_types=1);

$path = parse_url($_SERVER['REQUEST_URI'] ?? '/', PHP_URL_PATH) ?: '/';
// O Vercel pode entregar REQUEST_URI como /api/rota.php ou /rota.php,
// dependendo de como o rewrite chegou à função. Normalize os dois formatos.
$path = preg_replace('#^/api(?:/index\.php)?#', '', $path) ?? $path;
$path = preg_replace('#^/apipix(?:/index\.php)?#', '', $path) ?? $path;
$path = '/' . ltrim($path, '/');

$map = [
    '/gerar_pix.php' => 'gerar_pix.inc',
    '/verificar_pagamento.php' => 'verificar_pagamento.inc',
    '/webhook_flevopay.php' => 'webhook_flevopay.inc',
    '/webhook_winnerpay.php' => 'webhook_winnerpay.inc',
    '/webhook_pix.php' => 'webhook_pix.inc',
    '/gateway_status.php' => 'gateway_status.inc',
    '/gateway_config.php' => 'gateway_status.inc',
    '/configurar_webhook.php' => 'configurar_webhook.inc',
    '/_debug_ssl.php' => '_debug_ssl.inc',
];

if (isset($map[$path])) {
    require __DIR__ . '/../lib/' . $map[$path];
    exit;
}

/* Legacy BassPago/PIX API routes */
require_once __DIR__ . '/../lib/pix_api.inc';

header('Content-Type: application/json; charset=utf-8');
global $API_KEY;
$authHeader = $_SERVER['HTTP_AUTHORIZATION'] ?? '';
if (!hash_equals('Bearer ' . (string)$API_KEY, $authHeader)) {
    http_response_code(401);
    echo json_encode(['erro' => 'Não autorizado.']);
    exit;
}

$body = [];
if (in_array($_SERVER['REQUEST_METHOD'] ?? 'GET', ['POST','PUT','PATCH'], true)) {
    $raw=file_get_contents('php://input');
    if ($raw) $body=json_decode($raw,true) ?? [];
}
$uri = '/' . trim($path, '/');
try {
    $pix = new PixApi();
    echo json_encode(route($method=$_SERVER['REQUEST_METHOD'] ?? 'GET', $uri, $body, $pix), JSON_UNESCAPED_UNICODE|JSON_PRETTY_PRINT);
} catch (InvalidArgumentException $e) {
    http_response_code(400); echo json_encode(['erro'=>$e->getMessage()]);
} catch (RuntimeException $e) {
    $c=(int)$e->getCode(); http_response_code($c>=400&&$c<600?$c:500); echo json_encode(['erro'=>$e->getMessage()]);
} catch (Throwable $e) {
    http_response_code(500); echo json_encode(['erro'=>'Erro interno do servidor.']);
}

function route(string $method,string $uri,array $body,PixApi $pix):array {
    if($method==='GET'&&preg_match('#^/pix/([a-zA-Z0-9]{32})/devolucao/([a-zA-Z0-9]{1,35})$#',$uri,$m))return $pix->consultarDevolucao($m[1],$m[2]);
    if($method==='PUT'&&preg_match('#^/pix/([a-zA-Z0-9]{32})/devolucao/([a-zA-Z0-9]{1,35})$#',$uri,$m))return $pix->solicitarDevolucao($m[1],$m[2],$body);
    if($method==='GET'&&preg_match('#^/pix/([a-zA-Z0-9]{32})$#',$uri,$m))return $pix->consultarPix($m[1]);
    if($method==='GET'&&$uri==='/pix')return $pix->listarPix($_GET);
    if($method==='PATCH'&&preg_match('#^/cob/([a-zA-Z0-9]{26,35})$#',$uri,$m))return $pix->atualizarCobranca($m[1],$body);
    if($method==='PUT'&&preg_match('#^/cob/([a-zA-Z0-9]{26,35})$#',$uri,$m))return $pix->criarCobrancaComTxid($m[1],$body);
    if($method==='GET'&&preg_match('#^/cob/([a-zA-Z0-9]{26,35})$#',$uri,$m))return $pix->consultarCobranca($m[1],isset($_GET['revisao'])?(int)$_GET['revisao']:null);
    if($method==='POST'&&$uri==='/cob')return $pix->criarCobranca($body);
    if($method==='GET'&&$uri==='/cob')return $pix->listarCobrancas($_GET);
    if($method==='PUT'&&preg_match('#^/cobv/([a-zA-Z0-9]{26,35})$#',$uri,$m))return $pix->criarCobrancaVencimento($m[1],$body);
    if($method==='PATCH'&&preg_match('#^/cobv/([a-zA-Z0-9]{26,35})$#',$uri,$m))return $pix->atualizarCobrancaVencimento($m[1],$body);
    if($method==='GET'&&preg_match('#^/cobv/([a-zA-Z0-9]{26,35})$#',$uri,$m))return $pix->consultarCobrancaVencimento($m[1],isset($_GET['revisao'])?(int)$_GET['revisao']:null);
    if($method==='GET'&&$uri==='/cobv')return $pix->listarCobrancasVencimento($_GET);
    if($method==='PUT'&&preg_match('#^/webhook/(.+)$#',$uri,$m)){if(empty($body['webhookUrl']))throw new InvalidArgumentException('Campo webhookUrl é obrigatório.');return $pix->configurarWebhook(urldecode($m[1]),$body['webhookUrl']);}
    if($method==='GET'&&preg_match('#^/webhook/(.+)$#',$uri,$m))return $pix->consultarWebhook(urldecode($m[1]));
    if($method==='DELETE'&&preg_match('#^/webhook/(.+)$#',$uri,$m))return $pix->removerWebhook(urldecode($m[1]));
    if($method==='GET'&&preg_match('#^/loc/(\d+)$#',$uri,$m))return $pix->consultarLocation((int)$m[1]);
    if($method==='POST'&&$uri==='/loc'){if(empty($body['tipoCob']))throw new InvalidArgumentException('Campo tipoCob é obrigatório (cob ou cobv).');return $pix->criarLocation($body['tipoCob']);}
    http_response_code(404);return ['erro'=>'Rota não encontrada.'];
}
