<?php
/** Configuração do orquestrador persistida no Upstash Redis. */
class GatewayConfig
{
    private string $url;
    private string $token;
    private string $key = 'wbass:gateway_config';

    public function __construct()
    {
        global $UPSTASH_REDIS_REST_URL, $UPSTASH_REDIS_REST_TOKEN;
        $this->url = rtrim((string)($UPSTASH_REDIS_REST_URL ?? ''), '/');
        $this->token = (string)($UPSTASH_REDIS_REST_TOKEN ?? '');
    }

    public function get(): array
    {
        global $GATEWAY_MODE, $GATEWAY_MANUAL, $GATEWAY_FALLBACK;
        $default = [
            'mode' => strtolower((string)($GATEWAY_MODE ?? 'manual')) === 'automatico' ? 'automatico' : 'manual',
            'gateway' => in_array(strtolower((string)($GATEWAY_MANUAL ?? 'flevopay')), ['flevopay','basspago','winnerpay'], true) ? strtolower((string)$GATEWAY_MANUAL) : 'flevopay',
            'fallback' => (bool)($GATEWAY_FALLBACK ?? true),
        ];
        if ($this->url === '' || $this->token === '') return $default;
        try {
            $r = $this->command(['GET', $this->key]);
            $raw = $r['result'] ?? null;
            $saved = is_string($raw) ? json_decode($raw, true) : null;
            if (!is_array($saved)) return $default;
            return [
                'mode' => (($saved['mode'] ?? '') === 'automatico') ? 'automatico' : 'manual',
                'gateway' => in_array(($saved['gateway'] ?? ''), ['flevopay','basspago','winnerpay'], true) ? $saved['gateway'] : 'flevopay',
                'fallback' => (bool)($saved['fallback'] ?? true),
            ];
        } catch (Throwable $e) {
            return $default;
        }
    }

    public function save(string $mode, string $gateway, bool $fallback): array
    {
        if (!in_array($mode, ['manual', 'automatico'], true)) throw new InvalidArgumentException('Modo inválido.');
        if (!in_array($gateway, ['flevopay', 'basspago', 'winnerpay'], true)) throw new InvalidArgumentException('Gateway inválido.');
        if ($this->url === '' || $this->token === '') throw new RuntimeException('Upstash Redis não está configurado.');
        $config = ['mode'=>$mode, 'gateway'=>$gateway, 'fallback'=>$fallback, 'updated_at'=>date('c')];
        $this->command(['SET', $this->key, json_encode($config, JSON_UNESCAPED_UNICODE)]);
        return $this->get();
    }

    private function command(array $command): array
    {
        $ch = curl_init($this->url);
        curl_setopt_array($ch, [
            CURLOPT_RETURNTRANSFER => true,
            CURLOPT_TIMEOUT => 8,
            CURLOPT_POST => true,
            CURLOPT_HTTPHEADER => ['Authorization: Bearer '.$this->token, 'Content-Type: application/json'],
            CURLOPT_POSTFIELDS => json_encode($command),
        ]);
        $raw = curl_exec($ch);
        $err = curl_error($ch);
        $code = curl_getinfo($ch, CURLINFO_HTTP_CODE);
        curl_close($ch);
        if ($err) throw new RuntimeException('Redis: '.$err);
        if ($code < 200 || $code >= 300) throw new RuntimeException('Redis HTTP '.$code);
        $data = json_decode((string)$raw, true);
        return is_array($data) ? $data : [];
    }
}
