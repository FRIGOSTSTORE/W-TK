# Auditoria TikTok → UTMify

## Corrigido
- `index.html`: preservação de `ttclid` na passagem para `/checkout/pagar.html`; armazenamento em sessionStorage/localStorage; TikTok Pixel `D7T55DRC77U0A0BNCDKG`; evento `ViewContent`.
- `checkout/pagar.html`: `ttclid`, `fbclid` e `gclid` agora permanecem no conjunto de parâmetros enviados ao backend; `ttclid` é enviado explicitamente para `/api/gerar_pix.php`; eventos browser `InitiateCheckout` e `CompletePayment`.
- `lib/gateway_router.inc`: recebe e preserva `ttclid`; se houver `ttclid` sem `utm_source`, define `utm_source=tiktok` e `utm_medium=paid_social` sem sobrescrever UTMs fornecidas.
- `lib/gerar_pix.inc`: salva `ttclid` junto à transação no Upstash, para sobreviver ao redirecionamento/webhook.
- `lib/tracker.inc`: registra `ttclid` no log do envio à UTMify. O campo não é injetado em `trackingParameters`, evitando alterar o schema conhecido da API.
- Webhooks/polling: corrigida a ordem de lock → tracking → marcar como pago, reduzindo risco de corrida em que um caminho marcava a transação como paga antes de o outro enviar o `Purchase`.

## Observação importante
A UTMify informa oficialmente que possui tracking de TikTok e trackeamento server-side. A API de pedidos deste projeto continua usando `trackingParameters` com os campos que já estavam no schema (`utm_*`, `src`, `sck`). O `ttclid` é preservado localmente e usado para identificar a origem TikTok; não foi enviado como uma chave adicional à API sem confirmação do schema.

## Resultado esperado
TikTok → landing com `ttclid`/UTMs → checkout preserva parâmetros → `/api/gerar_pix.php` salva a atribuição → pagamento confirmado por webhook ou polling → `Tracker()->purchase()` envia `paid` para UTMify com os mesmos UTMs do pedido.
