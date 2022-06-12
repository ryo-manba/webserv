#ifndef SERVER_HPP
#define SERVER_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdexcept>

#include "Socket.hpp"

#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 8080
#define SIZE (5 * 1024)

class Server
{
private:
    struct sockaddr_in a_addr;
    Socket socket;
    int status_code;

    int request_size, response_size;
    char request_message[SIZE];
    char response_message[SIZE];
    char method[SIZE];
    char target[SIZE];
    char header_field[SIZE];
    char body[SIZE];
    unsigned int file_size;

public:
    Server(void)
    {
    }

    // 現状はsocketのセットアップだけだが、それ以外も増えることを考慮する
    void setup(void)
    {
        socket.setup();
        socket_wait_connection(3);
    }

    // backlogは sockfd についての保留中の接続のキューの最大長
    void socket_wait_connection(int backlog)
    {
        // ソケットを接続待ちに設定
        if (listen(socket.fd, backlog) == -1)
        {
            throw std::runtime_error("receive_socket error");
        }
    }

    void run()
    {
        printf("Waiting connect...\n");

        Socket send_socket(accept(socket.fd, NULL, NULL));

        printf("Contented!!\n");

        httpServer(send_socket.fd);
    }

    /*
     * リクエストメッセージを受信する
     * sock：接続済のソケット
     * request_message：リクエストメッセージを格納するバッファへのアドレス
     * buf_size：そのバッファのサイズ
     * 戻り値：受信したデータサイズ（バイト長）
     */
    int recvRequestMessage(int sock, char *request_message, unsigned int buf_size)
    {
        int recv_size;

        recv_size = recv(sock, request_message, buf_size, 0);
        if (recv_size == -1)
        {
            throw std::runtime_error("receive_socket error");
        }
        if (recv_size == 0)
        {
            // 受信サイズが0の場合は相手が接続を閉じていると判断
            //            throw std::runtime_error("connection ended");
            printf("connection ended\n");
        }

        return recv_size;
    }

    /*
     * リクエストメッセージを解析する（今回はリクエスト行のみ）
     * method：メソッドを格納するバッファへのアドレス
     * target：リクエストターゲットを格納するバッファへのアドレス
     * request_message：解析するリクエストメッセージが格納されたバッファへのアドレス
     * 戻り値：成功時は0、失敗時は-1
     */
    int parseRequestMessage(char *method, char *target, char *request_message)
    {
        char *line;
        char *tmp_method;
        char *tmp_target;

        // リクエストメッセージの1行のみ取得
        line = strtok(request_message, "\n");

        // " "までの文字列を取得しmethodにコピー
        tmp_method = strtok(line, " ");
        if (tmp_method == NULL)
        {
            throw std::runtime_error("get method error");
        }
        strcpy(method, tmp_method);

        // 次の " "までの文字列を取得しtargetにコピー
        tmp_target = strtok(NULL, " ");
        if (tmp_target == NULL)
        {
            throw std::runtime_error("get target error");
        }
        strcpy(target, tmp_target);

        return 0;
    }
    /*
     * リクエストに対する処理を行う（今回はGETのみ）
     * body：ボディを格納するバッファへのアドレス
     * file_path：リクエストターゲットに対応するファイルへのパス
     * 戻り値：ステータスコード（ファイルがない場合は404）
     */
    int getProcessing(char *body, char *file_path)
    {
        FILE *f;
        int file_size;

        // ファイルサイズを取得
        file_size = getFileSize(file_path);
        if (file_size == 0)
        {
            // ファイルサイズが0やファイルが存在しない場合は404を返す
            printf("getFileSize error\n");
            return 404;
        }

        // ファイルを読み込んでボディとする
        f = fopen(file_path, "r");
        fread(body, 1, file_size, f);
        fclose(f);

        return 200;
    }

    // ファイルサイズを取得する
    unsigned int getFileSize(const char *path)
    {
        int size, read_size;
        char read_buf[SIZE];
        FILE *f;

        f = fopen(path, "rb");
        if (f == NULL)
        {
            return 0;
        }

        size = 0;
        do
        {
            read_size = fread(read_buf, 1, SIZE, f);
            size += read_size;
        } while (read_size != 0);

        fclose(f);

        return size;
    }

    /*
     * レスポンスメッセージを作成する
     * response_message：レスポンスメッセージを格納するバッファへのアドレス
     * status：ステータスコード
     * header：ヘッダーフィールドを格納したバッファへのアドレス
     * body：ボディを格納したバッファへのアドレス
     * body_size：ボディのサイズ
     * 戻り値：レスポンスメッセージのデータサイズ（バイト長）
     */
    int createResponseMessage(char *response_message, int status_code, char *header, char *body, unsigned int body_size)
    {
        unsigned int no_body_len;
        unsigned int body_len;

        response_message[0] = '\0';

        if (status_code == 200)
        {
            // レスポンス行とヘッダーフィールドの文字列を作成
            sprintf(response_message, "HTTP/1.1 200 OK\r\n%s\r\n", header);

            no_body_len = strlen(response_message);
            body_len = body_size;

            // ヘッダーフィールドの後ろにボディをコピー
            memcpy(&response_message[no_body_len], body, body_len);
        }
        else if (status_code == 404)
        {
            // レスポンス行とヘッダーフィールドの文字列を作成
            sprintf(response_message, "HTTP/1.1 404 Not Found\r\n%s\r\n", header);

            no_body_len = strlen(response_message);
            body_len = 0;
        }
        else
        {
            // statusが200・404以外はこのプログラムで非サポート
            throw std::runtime_error("Not support status_code");
        }
        return no_body_len + body_len;
    }

    void showMessage(char *message, unsigned int size)
    {
        // コメントを外せばメッセージが表示される
        /*
            printf("Show Message\n\n");

            for (unsigned int i = 0; i < size; i++) {
                putchar(message[i]);
            }
            printf("\n\n");
        */
    }

    /*
     * レスポンスメッセージを送信する
     * sock：接続済のソケット
     * response_message：送信するレスポンスメッセージへのアドレス
     * message_size：送信するメッセージのサイズ
     * 戻り値：送信したデータサイズ（バイト長）
     */
    int sendResponseMessage(int sock, char *response_message, unsigned int message_size)
    {
        int send_size;

        send_size = send(sock, response_message, message_size, 0);

        return send_size;
    }

    void parseRequestMethod(char method[SIZE], char target[SIZE], char body[SIZE])
    {
        // メソッドがGET以外はステータスコードを404にする
        if (strcmp(method, "GET") == 0)
        {
            if (strcmp(target, "/") == 0)
            {
                // /が指定されたときはindex.htmlに置き換える
                strcpy(target, "/index.html");
            }

            // GETの応答をするために必要な処理を行う
            status_code = getProcessing(body, &target[1]);
        }
        else
        {
            status_code = 404;
        }
    }

    void createHeader(char header_field[SIZE], unsigned int file_size)
    {
        sprintf(header_field, "Content-Length: %u\r\n", file_size);
    }

    /*
     * HTTPサーバーの処理を行う関数
     * sock：接続済のソケット
     * 戻り値：0
     */
    void httpServer(int sock)
    {
        while (true)
        {
            // リクエスト・メッセージを受信
            request_size = recvRequestMessage(sock, request_message, SIZE);
            if (request_size == 0) // 0のとき接続終了
                break;
            // 受信した文字列を表示
            showMessage(request_message, request_size);

            // 受信した文字列を解析してメソッドやリクエストターゲットを取得
            parseRequestMessage(method, target, request_message);

            // メソッドの処理
            parseRequestMethod(method, target, body);

            file_size = getFileSize(&target[1]);

            // ヘッダーフィールド作成(今回はContent-Lengthのみ)
            createHeader(header_field, file_size);

            // レスポンスメッセージを作成
            response_size = createResponseMessage(response_message, status_code, header_field, body, file_size);

            // 送信するメッセージを表示(デバッグ)
            showMessage(response_message, response_size);

            // レスポンスメッセージを送信する
            sendResponseMessage(sock, response_message, response_size);
        }
    }
};
#endif
