#!/usr/bin/php

<?php

/*
** This program is alert info save
**
** Auther: Kazuo Ito
**
** Copyright (C) 2005-2014 ZABBIX-JP
** This program is licenced under the GPL
**/

# key word
$KEYWORD={キーワード};

# Separator
$SPL={セパレータ};

# log-file name
$FILE="/var/log/zabbix/alert";

# log-file extension
$EXT=".log";

######### Do not touch this line from ##########
$ALERT_INFO="";

mb_language('Japanese');
mb_internal_encoding('UTF-8');

$arr = explode("\n", $argv[3]);
foreach ( $arr as $value ) {
        $pos = strpos($value, $KEYWORD);
        if ($pos !== false) {
                $hosts = explode($SPL, $value);
                $ALERT_INFO=$FILE."_".$hosts[1].$EXT;
        }
}
if (empty($ALERT_INFO)) {
        $ALERT_INFO=$FILE."_".$EXT;
}

$fp = fopen($ALERT_INFO, "w");
fwrite($fp, $argv[2]);
fclose($fp);

$fp = fopen($ALERT_INFO, "a");
fwrite($fp, $argv[3]);
fclose($fp);

?>
