<?php
require_once __DIR__ . '/var.inc';
require_once __DIR__ . '/pix_api.inc';
require_once __DIR__ . '/gateway_config.inc';

class GatewayRouter {
    private array $gateways = ['flevopay', 'basspago', 'winnerpay'];

    public function create(array $i): array {
        $o = $this->order($i); $errors = [];
        foreach ($this->ordered() as $gw) {
            try {
                if ($gw === 'flevopay') {
                    if (($o['anonymous'] ?? false) === true) {
                        global $FLEVOPAY_ANON_NAME,$FLEVOPAY_ANON_EMAIL,$FLEVOPAY_ANON_DOCUMENT,$FLEVOPAY_ANON_PHONE;
                        $o['nome'] = (string)($FLEVOPAY_ANON_NAME ?? '');
                        $o['email'] = (string)($FLEVOPAY_ANON_EMAIL ?? '');
                        $o['document'] = preg_replace('/\D+/', '', (string)($FLEVOPAY_ANON_DOCUMENT ?? ''));
                        $o['phone'] = preg_replace('/\D+/', '', (string)($FLEVOPAY_ANON_PHONE ?? ''));
                    }
                    if (!$this->completeCustomer($o)) {
                        throw new RuntimeException('FlevoPay exige nome, e-mail, CPF/CNPJ e telefone.');
                    }
                    $r = $this->flevoCreate($o);
                } elseif ($gw === 'winnerpay') {
                    if (($o['anonymous'] ?? false) === true) {
                        global $FLEVOPAY_ANON_NAME,$FLEVOPAY_ANON_EMAIL,$FLEVOPAY_ANON_DOCUMENT,$FLEVOPAY_ANON_PHONE;
                        $o['nome'] = (string)($FLEVOPAY_ANON_NAME ?? '');
                        $o['email'] = (string)($FLEVOPAY_ANON_EMAIL ?? '');
                        $o['document'] = preg_replace('/\D+/', '', (string)($FLEVOPAY_ANON_DOCUMENT ?? ''));
                        $o['phone'] = preg_replace('/\D+/', '', (string)($FLEVOPAY_ANON_PHONE ?? ''));
                    }
                    if (!$this->completeCustomer($o)) {
                        throw new RuntimeException('WinnerPay exige nome, e-mail, CPF/CNPJ e telefone para este fluxo.');
                    }
                    $r = $this->winnerCreate($o);
                } else {
                    $r = $this->bassCreate($o);
                }
                $r['gateway'] = $gw; $r['order'] = $o;
                return $r;
            } catch (Throwable $e) {
                $errors[$gw] = $e->getMessage();
                error_log("[GatewayRouter] {$gw}: ".$e->getMessage());
                if (!$this->fallback()) break;
            }
        }
        throw new RuntimeException('Nenhum gateway conseguiu gerar o PIX: '.json_encode($errors, JSON_UNESCAPED_UNICODE), 502);
    }

    public function query(string $gw, string $txid): array {
        if ($gw === 'flevopay') return $this->flevoQuery($txid);
        if ($gw === 'basspago') return (new PixApi())->consultarCobranca($txid);
        if ($gw === 'winnerpay') return $this->winnerQuery($txid);
        throw new InvalidArgumentException('Gateway inválido.');
    }

    private function order(array $i): array {
        $v = round((float)($i['valor'] ?? 0), 2);
        if ($v < 0.01) throw new InvalidArgumentException('Informe um valor mínimo de R$ 0,01.');
        $o = [
            'valor'=>$v, 'descricao'=>mb_substr(trim((string)($i['descricao']??'Pagamento PIX')),0,140),
            'nome'=>trim((string)($i['nome']??'')), 'email'=>trim((string)($i['email']??'')),
            'document'=>preg_replace('/\D+/','',(string)($i['document']??'')),
            'phone'=>preg_replace('/\D+/','',(string)($i['phone']??'')), 'anonymous'=>(bool)($i['anonymous']??false),
            'url'=>(string)($i['url']??''), 'fbc'=>(string)($i['fbc']??''), 'fbp'=>(string)($i['fbp']??''),
            'ttclid'=>(string)($i['ttclid']??'')
        ];
        foreach (['utm_source','utm_medium','utm_campaign','utm_content','utm_term','src','sck','ttclid'] as $k) $o[$k]=(string)($i[$k]??'');
        // Se o clique veio do TikTok mas não trouxe utm_source, identifica a origem
        // pelo ttclid sem sobrescrever UTMs explicitamente fornecidas pelo anunciante.
        if ($o['ttclid'] !== '' && $o['utm_source'] === '') $o['utm_source'] = 'tiktok';
        if ($o['ttclid'] !== '' && $o['utm_medium'] === '') $o['utm_medium'] = 'paid_social';
        return $o;
    }

    private function ordered(): array {
        $cfg = (new GatewayConfig())->get();
        if ($cfg['mode'] === 'automatico') {
            $scores=['flevopay'=>$this->rate('flevopay'),'basspago'=>$this->rate('basspago'),'winnerpay'=>$this->rate('winnerpay')];
            arsort($scores); return array_keys($scores);
        }
        return array_values(array_unique(array_merge([$cfg['gateway']], $this->gateways)));
    }

    private function rate(string $gw): float {
        $f=__DIR__.'/gateway_stats.inc';
        if (is_file($f)) require_once $f;
        try { return class_exists('GatewayStats') ? (new GatewayStats())->rate($gw) : 0.0; }
        catch(Throwable $e){ return 0.0; }
    }

    private function fallback(): bool { return (bool)(new GatewayConfig())->get()['fallback']; }
    private function completeCustomer(array $o): bool {
        return $o['nome']!=='' && filter_var($o['email'],FILTER_VALIDATE_EMAIL)
            && strlen($o['document'])>=11 && strlen($o['phone'])>=10;
    }

    private function flevoCreate(array $o): array {
        global $FLEVOPAY_API_URL,$FLEVOPAY_SECRET_KEY;
        $ref='AJF'.substr(bin2hex(random_bytes(16)),0,29);
        $tracking=[];
        foreach(['utm_source','utm_medium','utm_campaign','utm_content','utm_term','src','sck'] as $k)
            if($o[$k]!=='') $tracking[$k]=$o[$k];

        $body=[
            'amount'=>(int)round($o['valor']*100),
            'description'=>$o['descricao']?:'Pagamento PIX',
            'reference'=>$ref,
            'source'=>'api_externa',
            'postback_url'=>$this->webhookUrl(),
            'customer'=>[
                'name'=>$o['nome'],'email'=>$o['email'],
                'document'=>$o['document'],'phone'=>$o['phone']
            ]
        ];
        if($tracking) $body['tracking']=$tracking;

        $r=$this->http('POST',rtrim($FLEVOPAY_API_URL,'/').'/api/v1/transaction',$body,
            ['X-API-Key: '.$FLEVOPAY_SECRET_KEY]);
        if(($r['status']??'')!=='success') throw new RuntimeException($r['message']??'FlevoPay recusou a transação.');
        $qr=$r['qr_code']??'';
        if($qr==='') throw new RuntimeException('FlevoPay não retornou qr_code.');
        return ['txid'=>$ref,'pixCopiaECola'=>$qr,'status'=>$r['status'],
            'valor'=>number_format($o['valor'],2,'.',''),'expiracao'=>3600,
            'flevopay_transaction_id'=>$r['transaction_id']??null];
    }

    private function flevoQuery(string $ref): array {
        global $FLEVOPAY_API_URL,$FLEVOPAY_SECRET_KEY;
        $u=rtrim($FLEVOPAY_API_URL,'/').'/api/v1/query?action=list_transactions&external_id='.rawurlencode($ref);
        $r=$this->http('GET',$u,null,['X-API-Key: '.$FLEVOPAY_SECRET_KEY]);
        return isset($r[0]) ? $r[0] : $r;
    }


    private function winnerCreate(array $o): array {
        global $WINNERPAY_API_URL,$WINNERPAY_CLIENT_ID,$WINNERPAY_CLIENT_SECRET;
        $tx = 'AJW'.substr(bin2hex(random_bytes(16)),0,29);
        $body = [
            'amount' => (float)number_format($o['valor'],2,'.',''),
            'description' => $o['descricao'] ?: 'Pagamento PIX',
            'postbackUrl' => $this->winnerWebhookUrl(),
            'external_id' => $tx,
            'metadata' => [
                'order_id' => $tx,
                'product' => ['name' => $o['descricao'] ?: 'Pagamento PIX']
            ],
            'customer' => [
                'name' => $o['nome'],
                'email' => $o['email'],
                'document' => ['type' => strlen($o['document']) === 14 ? 'CNPJ' : 'CPF', 'number' => $o['document']],
                'phone' => $o['phone']
            ]
        ];
        $headers = $this->winnerHeaders($WINNERPAY_CLIENT_ID,$WINNERPAY_CLIENT_SECRET);
        $r = $this->http('POST',rtrim($WINNERPAY_API_URL,'/').'/financial/receber-pix',$body,$headers);
        if (($r['success'] ?? false) !== true) throw new RuntimeException($r['message'] ?? 'WinnerPay recusou a transação.');
        $qr = (string)($r['pix_copia_e_cola'] ?? $r['qr_code_data'] ?? '');
        if ($qr === '') throw new RuntimeException('WinnerPay não retornou pix_copia_e_cola/qr_code_data.');
        $tid = $r['transaction']['transaction_id'] ?? $tx;
        return ['txid'=>$tx,'pixCopiaECola'=>$qr,'status'=>$r['transaction']['status']??'pending','valor'=>number_format($o['valor'],2,'.',''),'expiracao'=>3600,'winnerpay_transaction_id'=>$tid];
    }

    private function winnerQuery(string $txid): array {
        global $WINNERPAY_API_URL,$WINNERPAY_CLIENT_ID,$WINNERPAY_CLIENT_SECRET;
        $u=rtrim($WINNERPAY_API_URL,'/').'/financial/transactions?page=1&per_page=100';
        $r=$this->http('GET',$u,null,$this->winnerHeaders($WINNERPAY_CLIENT_ID,$WINNERPAY_CLIENT_SECRET));
        $rows=$r['transactions']['data']??$r['data']??[];
        if(!is_array($rows)) $rows=[];
        foreach($rows as $row){
            if((string)($row['transaction_id']??'')===$txid || (string)($row['external_id']??'')===$txid || (($row['metadata']['order_id']??'')===$txid)) return $row;
        }
        return ['status'=>'pending'];
    }

    private function winnerHeaders(string $id,string $secret): array {
        return ['Authorization: Basic '.base64_encode($id.':'.$secret)];
    }

    private function winnerWebhookUrl(): string {
        $host=$_SERVER['HTTP_HOST']??'';
        if($host==='') throw new RuntimeException('Domínio do webhook não identificado.');
        return 'https://'.$host.'/api/webhook_winnerpay.php';
    }

    private function bassCreate(array $o): array {
        global $CHAVE_PIX,$TXID_PREFIX;
        $p=preg_replace('/[^a-zA-Z0-9]/','',(string)($TXID_PREFIX??'AJD'));
        if($p==='')$p='AJD';
        $tx=$p.substr(bin2hex(random_bytes(16)),0,32-strlen($p));
        $d=['calendario'=>['expiracao'=>3600],
            'valor'=>['original'=>number_format($o['valor'],2,'.','')],
            'chave'=>trim($CHAVE_PIX)];
        if($o['descricao']!=='')$d['solicitacaoPagador']=$o['descricao'];
        $r=(new PixApi())->criarCobrancaComTxid($tx,$d);
        $qr=$r['pixCopiaECola']??$r['brcode']??$r['qrcode']??$r['emv']??($r['loc']['pixCopiaECola']??'');
        if($qr==='')throw new RuntimeException('BassPago não retornou PIX copia e cola.');
        return ['txid'=>$r['txid']??$tx,'pixCopiaECola'=>$qr,'status'=>$r['status']??'ATIVA',
            'valor'=>$r['valor']['original']??number_format($o['valor'],2,'.',''),
            'expiracao'=>$r['calendario']['expiracao']??3600];
    }

    private function webhookUrl(): string {
        $host=$_SERVER['HTTP_HOST']??'';
        if($host==='')throw new RuntimeException('Domínio do webhook não identificado.');
        return 'https://'.$host.'/api/webhook_flevopay.php';
    }

    private function http(string $method,string $url,?array $body,array $headers): array {
        $ch=curl_init($url); $h=array_merge(['Accept: application/json'],$headers);
        if($body!==null){$h[]='Content-Type: application/json';curl_setopt($ch,CURLOPT_POSTFIELDS,json_encode($body));}
        curl_setopt_array($ch,[CURLOPT_RETURNTRANSFER=>true,CURLOPT_TIMEOUT=>15,
            CURLOPT_CUSTOMREQUEST=>$method,CURLOPT_HTTPHEADER=>$h,CURLOPT_SSL_VERIFYPEER=>true,CURLOPT_SSL_VERIFYHOST=>2]);
        $raw=curl_exec($ch);$code=curl_getinfo($ch,CURLINFO_HTTP_CODE);$err=curl_error($ch);curl_close($ch);
        if($err)throw new RuntimeException('Erro HTTP: '.$err);
        $d=json_decode((string)$raw,true);
        if($code<200||$code>=300)throw new RuntimeException('HTTP '.$code.': '.(is_array($d)?json_encode($d):$raw));
        return is_array($d)?$d:[];
    }
}
