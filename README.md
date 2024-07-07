# TrigaMirror


Crie uma pasta chamada trigamirror em /etc
```Bash
sudo mkdir -p /etc/trigamirror
```

Crie quantos arquivos forem necessários chamados trigamirror-N.conf onde N é um número. Use um editor de texto, por exemplo, o GNU Nano:

```Bash
sudo nano /etc/trigamirror/trigamirror-1.conf
```

Aonde o conteúdo do arquivo seria:

```Conf
SERVER_IP=127.0.0.1
SERVER_PORT=123
READ_TAX=10
MIRROR_PORT=123
MIRROR_HEADER=0
```

Garanta que os arquivos criados tenham as permições corretas:

```Bash
sudo chmod 644 /etc/trigamirror/trigamirror-*.conf
```

Atualize o daemon:

```Bash
sudo systemctl daemon-reload
```

Ative e este cada um dos serviços:

```Bash
sudo systemctl start trigamirror@1
sudo systemctl status trigamirror@1
```

Para deixar ativo durante a inicialização, execute:

```Bash
sudo systemctl enable trigamirror@1
```
