<?php
/****************/
/* By Tim Greve */
/****************/

if($argc != 4) die("Wrong parameter count. Please provice the language as first, the voice as second and the string to say as thrid parameter. E. g.: IvonaTTS.php de-DE Marlene \"Hello World\"");

$language = $argv[1];
if($language == 'de') $language = 'de-DE';
else if($language == 'fr') $language = 'fr-FR';
else if($language == 'en') $language = 'en-US';
$voice = $argv[2];
$path = \Homegear\Homegear::TEMP_PATH."sonos/";

if($language == 'de-DE' && $voice != 'Marlene' && $voice != 'Hans') $voice = 'Marlene';
else if($language == 'en-US' && $voice != 'Justin' && $voice != 'Salli' && $voice != 'Joey' && $voice != 'Kimberly' && $voice != 'Kendra' && $voice != 'Eric' && $voice != 'Jennifer' && $voice != 'Ivy' && $voice != 'Chipmunk') $voice = 'Justin';
else if($language == 'en-GB' && $voice != 'Amy' && $voice != 'Brian' && $voice != 'Emma') $voice = 'Amy';
else if($language == 'fr-FR' && $voice != 'Celine' && $voice != 'Mathieu') $voice = 'Celine';

if(!file_exists($path))
{
	if(!mkdir($path, 0775, true)) die("Could not create directory $path");
}

IVONA_TTS::$accessKey = \Homegear\Homegear::getFamilySetting(6, 'ivonattsaccesskey');
IVONA_TTS::$secretKey = \Homegear\Homegear::getFamilySetting(6, 'ivonattssecretkey');
if(!IVONA_TTS::$accessKey || !IVONA_TTS::$secretKey) die("Please specify \"ivonaTtsAccessKey\" and \"ivonaTtsSecretKey\" in \"sonos.conf\".");

$ivona = new IVONA_TTS($accessKey, $secretKey);

$words = $argv[3]; //escapeshellarg($argv[2]);
$filename = $path.md5($words)."-".$language."-".$voice.".mp3";
if(file_exists($filename) && filesize($filename) > 1024) touch($filename);
else $ivona->save_mp3($words, $filename, $language, $voice);

echo $filename;

exit(0);


///////////////////////////////////////////////////////////////////////////////////////
//   IVONA_TTS             ////////////////////////////////////////////////////////////
//    by Titus 15.10.2015  ////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

class IVONA_TTS
{
    static  $utc_tz;
    static  $accessKey = '';
    static  $secretKey = '';

    function save_mp3($text, $filename, $language="de-DE", $voice="Marlene", $rate="default", $volume="default") {
         $payload['Input'] = array();
            $payload['Input']['Data'] = $text; //utf8_encode($text);//{"Input":{"Data":"Hello world"}}'
            $payload['Parameters']['Rate'] = $rate;
            $payload['Parameters']['Volume'] = $volume;
            $payload['Voice']['Name'] = $voice;
            $payload['Voice']['Language'] = $language;

            $payload = json_encode($payload);
            $mp3 = $this->get_mp3($payload);
            file_put_contents($filename, $mp3);
     }

    function get_mp3( $payload )
    {
        if( !self::$utc_tz ) {
            self::$utc_tz = new \DateTimeZone( 'UTC' );
          }
        $datestamp  = new \DateTime( "now", self::$utc_tz );
        $longdate   = $datestamp->format( "Ymd\\THis\\Z");
        $shortdate  = $datestamp->format( "Ymd" );
        // establish the signing key
        {
            $ksecret    = 'AWS4' . self::$secretKey;
            $kdate      = hash_hmac( 'sha256', $shortdate, $ksecret, true );
            $kregion    = hash_hmac( 'sha256', 'eu-west-1', $kdate, true );
            $kservice   = hash_hmac( 'sha256', 'tts', $kregion, true );
            $ksigning   = hash_hmac( 'sha256', 'aws4_request', $kservice, true );
        }
        // command parameters
        $params     = array(
            'host'          => 'tts.eu-west-1.ivonacloud.com',
            'content-type'  => 'application/json',
            'x-amz-content-sha256' => hash( 'sha256', $payload ),
            'x-amz-date'    => $longdate,
        );
        $canonical_request  = $this->createCanonicalRequest( $params, $payload );
        $signed_request     = hash( 'sha256', $canonical_request );
        $sign_string        = "AWS4-HMAC-SHA256\n{$longdate}\n$shortdate/eu-west-1/tts/aws4_request\n" . $signed_request;
        $signature          = hash_hmac( 'sha256', $sign_string, $ksigning );
        $params['Authorization'] = "AWS4-HMAC-SHA256 Credential=" . self::$accessKey . "/$shortdate/eu-west-1/tts/aws4_request, " .
                                   "SignedHeaders=content-type;host;x-amz-content-sha256;x-amz-date, " .
                                   "Signature=$signature";
          $params['content-length'] = strlen( $payload ) ;
        /*
         * Execute Crafted Request
         */
        $url    = "https://tts.eu-west-1.ivonacloud.com/CreateSpeech";
        $ch     = curl_init();
        $curl_headers = array();
        foreach( $params as $p => $k )
            $curl_headers[] = $p . ": " . $k;
        curl_setopt($ch, CURLOPT_URL,$url);
        curl_setopt($ch, CURLOPT_POST,1);
        curl_setopt($ch, CURLOPT_POSTFIELDS, $payload);
        curl_setopt($ch, CURLOPT_HTTPHEADER, $curl_headers);
        curl_setopt($ch, CURLOPT_RETURNTRANSFER,1);
        curl_setopt($ch, CURLOPT_TCP_NODELAY, true);
        curl_setopt($ch, CURLOPT_SSL_VERIFYHOST, 2 );
        curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, true );
        // debug opts
        {
            curl_setopt($ch, CURLOPT_VERBOSE, true);
            $verbose = fopen('php://temp', 'rw+');
            curl_setopt($ch, CURLOPT_STDERR, $verbose);
            $result = curl_exec($ch); // raw result
            rewind($verbose);
            $verboseLog = stream_get_contents($verbose);
            #echo "Verbose information:\n<pre>", htmlspecialchars($verboseLog), "</pre>\n";
        }
        return $result;
    }
    private function createCanonicalRequest( Array $params, $payload )
    {
        $canonical_request      = array();
        $canonical_request[]    = 'POST';
        $canonical_request[]    = '/CreateSpeech';
        $canonical_request[]    = '';
        $can_headers            = array(
          'host' => 'tts.eu-west-1.ivonacloud.com'
        );
        foreach( $params as $k => $v )
            $can_headers[ strtolower( $k ) ] = trim( $v );
        uksort( $can_headers, 'strcmp' );
        foreach ( $can_headers as $k => $v )
            $canonical_request[] = $k . ':' . $v;
        $canonical_request[] = '';
        $canonical_request[] = implode( ';', array_keys( $can_headers ) );
        $canonical_request[] = hash( 'sha256', $payload );
        $canonical_request = implode( "\n", $canonical_request );
        return $canonical_request;
    }
}
