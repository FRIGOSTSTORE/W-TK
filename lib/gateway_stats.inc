<?php
require_once __DIR__.'/var.inc';
class GatewayStats {
    private string $url; private string $token;
    public function __construct(){global $UPSTASH_REDIS_REST_URL,$UPSTASH_REDIS_REST_TOKEN;$this->url=rtrim($UPSTASH_REDIS_REST_URL??'','/');$this->token=$UPSTASH_REDIS_REST_TOKEN??'';}
    public function increment(string $gw,string $kind):void{
        if(!$this->url||!$this->token)return;
        $key='gw:'.$gw.':'.($kind==='paid'?'paid':'generated').':'.date('YmdH');
        $this->cmd(['INCR',$key]);$this->cmd(['EXPIRE',$key,'172800']);
    }
    public function rate(string $gw):float{
        $g=0;$p=0;$now=time();
        for($i=0;$i<3;$i++){ $h=date('YmdH',$now-$i*3600);
            $g+=(int)($this->cmd(['GET',"gw:$gw:generated:$h"])['result']??0);
            $p+=(int)($this->cmd(['GET',"gw:$gw:paid:$h"])['result']??0);
        }
        return $g>0?$p/$g:0.0;
    }
    private function cmd(array $a):array{
        $c=curl_init($this->url);curl_setopt_array($c,[CURLOPT_RETURNTRANSFER=>true,CURLOPT_TIMEOUT=>5,CURLOPT_POST=>true,
        CURLOPT_HTTPHEADER=>['Authorization: Bearer '.$this->token,'Content-Type: application/json'],CURLOPT_POSTFIELDS=>json_encode($a)]);
        $r=curl_exec($c);curl_close($c);$d=json_decode((string)$r,true);return is_array($d)?$d:[];
    }
}
