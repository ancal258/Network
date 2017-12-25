#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <Windows.h>
#include <math.h>
#define SERVERPORT 9000
#define BUFSIZE    1024
#define CLIENTCOUNT 2
#define max 4

typedef struct Vector3
{
	float x= -1, y, z;
	bool state;
	bool live = false;
}Vector3;
typedef struct Bg
{
	double x;
	double y;
	double z;
}Bg;
typedef struct ServerToClient_info
{

	int clientCount = 0;
	int fromClientBossHP; // �������� ���� HP ó��
	bool fromClientRabbit_C; // �ٸ� Ŭ�� ������ �浹 �� ���  --> True ����
	bool bullet_state;
	bool stageInit;
	bool checkCollision=false;
	Vector3 player_vector;
	Bg boss_data;
}ServerToClient_info;

typedef struct Sever_Data
{
	ServerToClient_info data[max];
}Sever_Data;

Sever_Data server_data;

typedef struct SocketInfo
{
	int clientNum;
	SOCKET socket;
}SI;

//���� ��� ����
CRITICAL_SECTION cs;

SOCKET client[CLIENTCOUNT];

int countCheck = 0; // send num check
int ID = 0;           //���� ��ȣ ���̵�
Bg p;

bool playerGet[2] = { false };  // ��� �ϳ� ������� 

//�̺�Ʈ �ڵ� ����
HANDLE event[3];
int recvn(SOCKET s, char *buf, int len, int flags)
{
	int received;
	char *ptr = buf;
	int left = len;

	while (left > 0) {
		received = recv(s, ptr, left, flags);
		if (received == SOCKET_ERROR)
			return SOCKET_ERROR;
		else if (received == 0)
			break;
		left -= received;
		ptr += received;
	}
	//   printf("���� �Ǵ�? ");
	return (len - left);
}

int count = 0;
// ���� �Լ� ���� ��� �� ����
void err_quit(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, 0, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

// ���� �Լ� ���� ���
void err_display(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", msg, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

bool checkCollision(Sever_Data a)
{

	//��� Ŭ�� �浹 �ʱ�ȭ
	for ( int k = 0; k < count; ++k )
		server_data.data[k].checkCollision = false;

	for ( int j = 1; j < count + 1; ++j ) {
		for ( int i = 0; i < count; ++i ) {

			// �������� ���� Ŭ��� break �������� vector.x�� ���� -1�� �ʱ�ȭ ���ѳ���
			if ( a.data[i + j].player_vector.x == -1 )
				break;

			// �浹 ����
			if ( 3 > sqrt( (a.data[i].player_vector.x - a.data[i + j].player_vector.x)*(a.data[i].player_vector.x - a.data[i + j].player_vector.x)
				+ (a.data[i].player_vector.z - a.data[i + j].player_vector.z)*(a.data[i].player_vector.z - a.data[i + j].player_vector.z) ) ) {
				
				server_data.data[i].checkCollision = true;
				server_data.data[i+j].checkCollision = true;
				
				// �ε����� �� �ణ�� �̵�
				if ( server_data.data[i].player_vector.x > server_data.data[i+j].player_vector.x ) {
					server_data.data[i].player_vector.x += 0.05;

				}
				else {
					server_data.data[i + j].player_vector.x += 0.05;
				}
				

				if (server_data.data[i].player_vector.x < server_data.data[i + j].player_vector.x) {
					server_data.data[i].player_vector.x -= 0.05;

				}
				else {
					server_data.data[i + j].player_vector.x -= 0.05;
				}
				return true;
			}
		}
	}
}

// Ŭ���̾�Ʈ�� ������ ���
DWORD WINAPI ConnectThread(LPVOID arg)
{

	SOCKET client_sock = (SOCKET)arg;
	int retval;
	


	//�̺�Ʈ ����
	if(count ==0)
	    event[0] = CreateEvent(NULL, TRUE, TRUE, NULL);
	if(count ==1)
		event[1] = CreateEvent(NULL, TRUE, FALSE, NULL);
	if(count==2)
		event[2] = CreateEvent(NULL, TRUE, FALSE, NULL);


	SOCKADDR_IN clientaddr;
	int addrlen;
	char buf[BUFSIZE + 1];
	int len;
	//client[temp] = client_sock;
	// Ŭ���̾�Ʈ ���� ���
	addrlen = sizeof(clientaddr);
	getpeername(client_sock, (SOCKADDR *)&clientaddr, &addrlen);

	int threadNum = count++;
	printf("%d ��° Ŭ�� ���� \n", threadNum);


	//���� �ð� �޾ƿ�
	int tmpcount = 0;
	int StartTime;
	StartTime = GetTickCount();


	// ������ ������� thread �ѹ��� Ŭ�� ���̵� ������.
	char client_id[10];
	itoa( threadNum, client_id, 10 );
	retval = send( client_sock, (char*)&client_id, sizeof( client_id ), 0 );
	if ( retval == SOCKET_ERROR ) {
		err_display( "send()" );
		//exit( 1 );
	}

	while (1) {

		if(threadNum ==0)
		WaitForSingleObject(event[2], INFINITE);

		if (threadNum == 1)
			WaitForSingleObject(event[0], INFINITE);

		if (threadNum == 2)
			WaitForSingleObject(event[1], INFINITE);


		
		// 33������ ����
		if (GetTickCount() - StartTime >= 30)
		{

			// Ŭ�� ������ �ޱ�
			ServerToClient_info *getdata;
			char suBuffer[500];
			int count = 0;

			retval = recv(client_sock, suBuffer, sizeof(ServerToClient_info), 0);
			if (retval == SOCKET_ERROR) {
				err_display("send()");
				break;
			}

			suBuffer[retval] = '\0';

			getdata = (ServerToClient_info*)suBuffer;
			server_data.data[threadNum].clientCount = threadNum;
			server_data.data[threadNum].boss_data = getdata->boss_data;
			server_data.data[threadNum].bullet_state = getdata->bullet_state;
			server_data.data[threadNum].clientCount = getdata->clientCount;
			server_data.data[threadNum].player_vector = getdata->player_vector;
			//
			server_data.data[threadNum].fromClientBossHP = getdata->fromClientBossHP;
			server_data.data[threadNum].fromClientRabbit_C = getdata->fromClientRabbit_C;
			server_data.data[threadNum].stageInit = getdata->stageInit;


			// �浹üũ
			checkCollision(server_data);

			// ���� ������ ������
			retval = send(client_sock, (char*)&server_data, sizeof(Sever_Data), 0);
			if (retval == SOCKET_ERROR) {
				err_display("send()");
				break;
			}


			
				if (threadNum == 0)
					SetEvent(event[1]);
				else if (threadNum == 1)
					SetEvent(event[2]);
				else if (threadNum == 2)
					SetEvent(event[0]);
			}
		
		
	}

	printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
		inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
	// closesocket()
	closesocket(client_sock);
	return 0;
}

int main(int argc, char *argv[])
{
	int retval;
	
	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	
	//SetEvent(event);


	// socket()
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");

	// bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");

	// ������ ��ſ� ����� ����
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;
	HANDLE hThread;   // Ŀ��Ʈ �ڵ�
	

	while (1) {
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (SOCKADDR *)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}
		client[count] = client_sock;   //������� ����
									   // ������ Ŭ���̾�Ʈ ���� ���
		printf("\n[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
			inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
		hThread = CreateThread(NULL, 0, ConnectThread,
			(LPVOID)client[count], 0, NULL);
		if (hThread == NULL) { closesocket(client[count]); }
		else { CloseHandle(hThread); }

	}



	// closesocket()
	closesocket(listen_sock);

	// ���� ����
	WSACleanup();
	return 0;
}