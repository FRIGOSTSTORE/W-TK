<?php

$URL_API = "https://api.pix.basspago.com.br";
$SENHA_CASH_IN = "d.WWxgdpH_r3nv2HCMJcYxH-K8xkJqPJJuAH7V2Tp2CNbugdDWc!nbvH7a2X";
$CLIENT_ID = "00011193760124794000127";
$CLIENT_SECRET = "mNhMDcwNTItYTc4Ni00MjE1LThlOGEtZ";
 
$CHAVE_PIX = "fcd47d4c-bd68-440a-8480-9c5a6c184abc";

// ── Prefixo de txid (conta BassPago compartilhada) ────────────────────────────
// Esta conta BassPago é usada por mais de um projeto (Norisk, site-elseve,
// ajude-main). O webhook é único por conta, então qualquer projeto pode
// receber notificações de txid de outro. Este prefixo é gravado no início
// de todo txid gerado por ESTE projeto (gerar_pix.php) e é conferido no
// webhook (webhook_pix.php) antes de processar qualquer pagamento — segue o
// mesmo padrão do prefixo NRK usado pelo Norisk.
$TXID_PREFIX = "AJD";
 
// ── Facebook Conversions API ──────────────────────────────────────────────────
// Todos os Pixels que devem receber os eventos da Conversions API.
// Para adicionar outro Pixel, inclua mais um bloco nesta lista.
$FB_PIXELS = [
    [
        'id' => '2586186688499394',
        'access_token' => 'EAAkGrZCdgqlMBSdEGhoS5ZCzF8kIYmFrxGlwpbXcuTgnsXRkl6Q7B9sw9HVG8IkA5btg4RDONNPcO7SruOio7MzYUL7OKrmZBz186fcU3GaZAoZAqcUZBpO9P3nZCZBvz2zanMiQPDaOD4wL3NUSCv7sX8ZBCepJaMtOAZCisfZBKiHo6adJOzrBEHl76qnEuNeEgZDZD',
    ],
    [
        'id' => '1762981111690468',
        'access_token' => 'EAAUJvQOQFFABSFRpx0DmZC9QlMwrSD0RNFz6YXZAj2E2SZAZCeuKCL5X3W1ZAfZARPTpMLUo4bvPEOBvPzrZCRvHqlVqDPkgAj9cNVXPvIAScCtg57ZAPZAGHDAFY8zW4pSb3GZBTWZAR6FEixgneq9PHDeJdujdFwpKqZCSkMuxbKlZCuTDLOtrwJhoGoveUKoryXwZDZD',
    ],
];
 
// ── UTMify ────────────────────────────────────────────────────────────────────
$UTMIFY_API_TOKEN = "43JMlfSLoN3IGSGLD4qeC8TDts9UzXaWXFfB";
$NOME_PRODUTO = "01 W Produto Digital";
$ID_PRODUTO   = "produto-001";
 
$API_KEY = "a4f5d95862b7c5238cd957db82b3482e0b7de358fe65c50f5c6d08985f85dd3c";

// ── Upstash Redis (armazenamento de transações PIX — necessário na Vercel) ────
// Crie uma conta grátis em https://upstash.com, crie um banco Redis
// e cole aqui a REST URL e o REST TOKEN que aparecem no painel do banco.
$UPSTASH_REDIS_REST_URL   = "https://secure-ape-166176.upstash.io";
$UPSTASH_REDIS_REST_TOKEN = "gQAAAAAAAokgAAIgcDFmYmYyZjhmY2JlYzc0ZGFiOTU5OGRmZjUyZWRhZGVkZA";


// ── Orquestrador multigateway ────────────────────────────────────────────────
$GATEWAY_MODE = "manual";          // manual | automatico
$GATEWAY_MANUAL = "flevopay";     // flevopay | basspago
$GATEWAY_FALLBACK = true;
$GATEWAY_MIN_FLOOR = 0.05;
$GATEWAY_WINDOW_SECONDS = 180 * 60;

// FlevoPay
$FLEVOPAY_STORE_ID = "10722";
$FLEVOPAY_SECRET_KEY = "flevopay_sk_5da292b0c812ecd6884fc56f8739a7ffecb2a8a85205a2b0eac2f42289ed25e9";
$FLEVOPAY_API_URL = "https://app.flevopay.com.br";

$FLEVOPAY_ANON_NAME = "JHON EXEMPLO";
$FLEVOPAY_ANON_EMAIL = "PIXDACASA@GMAIL.COM";
$FLEVOPAY_ANON_DOCUMENT = "13288630427";
$FLEVOPAY_ANON_PHONE = "11998787876";

// WinnerPay
$WINNERPAY_API_URL = getenv('WINNERPAY_API_URL') ?: 'https://api.winnerpayy.com.br/api';
$WINNERPAY_CLIENT_ID = getenv('WINNERPAY_CLIENT_ID') ?: 'a1f0d798-0c3c-45c8-9cc5-e6cae08f63f2';
$WINNERPAY_CLIENT_SECRET = getenv('WINNERPAY_CLIENT_SECRET') ?: '0d08e2e24ec230c1d6361f4edc7b5d108fc2e94f33aabd13457d9e7bef9f10c7';
