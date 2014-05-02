マップで障害の起きているサーバをクリックし、「障害情報」を選ぶとサブウインドが開きメールの中身が表示されます。
※：イメージはimg1.png / img2.png参照

1)zabbix_server.confのAlertScriptsPathにAlertInfo.shを配置する。  


2)Zabbixサーバのzabbix_agentd.confのEnableRemoteCommandsをEnableRemoteCommands=1にする。  


3)[管理]-[メディアタイプ]で新しいメディアタイプを作成し、AlertInfo.shを設定する  

    説明        ：AlaertInfo
    タイプ      ：スクリプト
    スクリプト名：AlertInfo.sh
    有効        ：チェックon


4)[管理]-[ユーザ]のメンバーを選択し、[メディア]タブを開いてAlaertInfoを追加する。  

    タイプ                    ：AlaertInfo
    送信先                    ：使わないので何でもいいのですが、何か設定してください。
    有効な時間帯              ：1-7,00:00-24:00
    指定した深刻度のときに使用：全部チェックon
    ステータス                ：有効


5)[管理]-[スクリプト]で[スクリプト作成]をクリックする  

    名前     ：障害情報
    タイプ   ：スクリプト
    次で実行 ：Zabbixサーバ
    コマンド ：cat /var/log/zabbix/alert_{HOSTNAME}.log


尚、AlertInfo.sh内でホスト名を取得するのでメール中にホスト名を「{キーワード}{セパレータ}{ホスト名}」という形で入れておく必要があります。  
私の環境では↓こんな感じでメールに入れていたので…  

AlertInfo.shは下記の用に書いてますので  

    1)ホスト名　：{HOSTNAME}

AlertInfo.shに{キーワード}{セパレータ}を下記のように指定してください。  
    # key word
    $KEYWORD="1)ホスト名";
     
    # Separator
    $SPL="：";
