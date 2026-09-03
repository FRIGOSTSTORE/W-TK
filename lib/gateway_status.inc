<?php
require_once __DIR__.'/var.inc';
require_once __DIR__.'/gateway_stats.inc';
require_once __DIR__.'/gateway_config.inc';
header('Content-Type: application/json; charset=utf-8');

$configStore = new GatewayConfig();
try {
    if ($_SERVER['REQUEST_METHOD'] === 'POST') {
        $body = json_decode(file_get_contents('php://input'), true) ?? [];
        $config = $configStore->save(
            (string)($body['mode'] ?? 'manual'),
            (string)($body['gateway'] ?? 'flevopay'),
            (bool)($body['fallback'] ?? true)
        );
    } elseif ($_SERVER['REQUEST_METHOD'] === 'GET') {
        $config = $configStore->get();
    } else {
        http_response_code(405);
        echo json_encode(['erro'=>'Método não permitido.'], JSON_UNESCAPED_UNICODE);
        exit;
    }

    $stats = new GatewayStats();
    echo json_encode([
        'ok'=>true,
        'config'=>$config,
        'stats'=>[
            'flevopay'=>$stats->rate('flevopay'),
            'basspago'=>$stats->rate('basspago'),
            'winnerpay'=>$stats->rate('winnerpay'),
        ],
    ], JSON_UNESCAPED_UNICODE);
} catch (Throwable $e) {
    http_response_code(500);
    echo json_encode(['erro'=>$e->getMessage()], JSON_UNESCAPED_UNICODE);
}
