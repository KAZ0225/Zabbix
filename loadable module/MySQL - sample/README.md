<<<はじめに>>>  
このソースはZabbixのローダブルモジュールからMySQLのshow statusコマンドを実行するものです。  
アイテムにkaz.mysql[キーワード]を設定すると、キーワードにしたshow statusのVariable_nameに対するvalueを取得します。

尚、MySQLへの接続情報は/etc/zabbix/loadable.confに指定します。  
また、一度MySQLへ接続すると接続エラーが発生するまで接続を維持します。  
接続エラーになった場合は、次の監視タイミングで再接続します。  

設定ファイルのIntervalですが、指定した秒数の間はSQLを発行せずにメモリに取得した値を返却します。  
※：値の取得は１回ですべての値を取得しメモリに保持します。  
Intervalに0を設定すると接続のたびにSQLを発行し、値を取得します。  

<<<コンパイル 〜 起動の仕方 … CentOS6.x用>>>  

1)Zabbix2.2.2 or Zabbix.2.2.3のソースを取得します。  


2)ソースを解凍  

    ]# tar zxf zabbix-2.2.3.tar.gz


3)ローダブルモジュールのソース・Makefileを配置  

    ]# mkdir zabbix-2.2.3/src/modules/kaz

  上記作ったDirにkaz.cとMakefileを置きます  


4)Zabbixエージェントのコンパイル  
ローダブルモジュールのコンパイルに本体コンパイル時に作られるヘッダファイルが必要な為  

    ]# ./configure --enable-agent
    ]# make

※：make installは不要  

5)ローダブルモジュールのコンパイル
    ]# cd zabbix-2.2.3/src/modules/kaz
    ]# make

実行するとこんな感じの表示が出ます。

    ]# make
    gcc -shared -o kaz.so kaz.c \
    		-fPIC \  
    		-I../../../include \  
    		-L/usr/lib64/mysql \  
    		-lmysqlclient  

エラーの場合はこんな感じにエラーが出ます。  

    ]# make  
    gcc -shared -o kaz.so kaz.c \  
    		-fPIC \  
    		-I../../../include \  
    		-L/usr/lib64/mysql \  
    		-lmysqlclient  
    kaz.c: In function ‘zbx_module_trim’:  
    kaz.c:822: error: expected ‘;’ before ‘}’ token  
    make: *** [kaz] エラー 1  

※：このエラーは『kaz.c : 822行目 : エラー : '}'記号(token)の前(before)に'；'を期待(expecter)する』と言う内容です。  

6)zabbix_agent.confを修正します。  
LoadModulePathとLoadModuleを設定します。  
LoadModulePathはローダブルモジュールの配置先  
LoadModuleはコンパイルしてできたオブジェクトファイル名  

========= /etc/zabbix/zabbix_agentd.conf 修正例 ==============  

    --略--  
    ####### LOADABLE MODULES #######  
     
    ### Option: LoadModulePath  
    #       Full path to location of agent modules.  
    #       Default depends on compilation options.  
    #  
    # Mandatory: no  
    # Default:  
    # LoadModulePath=${libdir}/modules  
    LoadModulePath=/etc/zabbix/modules  
     
    ### Option: LoadModule  
    #       Module to load at agent startup. Modules are used to extend functionality of the agent.  
    #       Format: LoadModule=<module.so>  
    #       The modules must be located in directory specified by LoadModulePath.  
    #       It is allowed to include multiple LoadModule parameters.  
    #  
    # Mandatory: no  
    # Default:  
    # LoadModule=  
    LoadModule=kaz.so  
    --略--  

========= /etc/zabbix/zabbix_agent.conf ==============  

7)Zabbixエージェントを停止します。(止まっていたら不要)  

    ]# service zabbix-agent stop  

8)コンパイルしてできたオブジェクトファイルを配置します。  

9)Zabbixエージェントを起動します。  

    ]# service zabbix-agent start  

※：ログに「loaded modules: xxxx.so」と出ていればローダブルモジュールが正常にロードされました。  
========= Zabbixエージェントのログ =========  

      1334:20140429:062917.504 Starting Zabbix Agent [Zabbix server]. Zabbix 2.2.3 (revision 44105).  
      1334:20140429:062917.507 using configuration file: /etc/zabbix/zabbix_agentd.conf  
      1334:20140429:062917.521 loaded modules: kaz.so  
      1349:20140429:062917.550 agent #1 started [listener #1]  
      1350:20140429:062917.550 agent #2 started [listener #2]  
      1351:20140429:062917.550 agent #3 started [listener #3]  
      1352:20140429:062917.551 agent #4 started [active checks #1]  
      1348:20140429:062917.552 agent #0 started [collector]  

========= Zabbixエージェントのログ =========  

10)実行例  

    ]# zabbix_get -s 127.0.0.1 -k "kaz.mysql[Connections]"  
    67  
    ]# zabbix_get -s 127.0.0.1 -k "kaz.mysql[Slave_running]"  
    OFF  
    ]# zabbix_get -s 127.0.0.1 -k "kaz.mysql[Threads_cached]"  
    0  
    ]# zabbix_get -s 127.0.0.1 -k "kaz.mysql[Innodb_buffer_pool_read_requests]"  
    33306  
