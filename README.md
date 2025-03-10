# Spider Search Engine


������ ������������ ����� ������� ��������� �������, ������������� ��� ������� ������ �� C++, ��� �������� ���-�������� � ������.\
������� �� ���� �����������:
- **SpiderApp**: ���-����, ���������� ������ � ���-�������, ������������� �� � ����������� � ���� ������ PostgreSQL.
- **HttpServerApp**: HTTP-������, ��������������� ���-��������� ��� ������ �� ��������� ������.

> [!NOTE]
> ������ ���������� � ������������� ������ ��� Windows. ������������� � ������� ������������� ��������� �� �������������.

## �����������
- **SpiderApp**: ������ �� 10,000 ���������� URL � �������� ��������, ��������� ����� � ������� ����, ��������� ���������� � PostgreSQL.
- **HttpServerApp**: ������������ ��������� ������� (1�4 �����, ������ 3�32 �������), ���������� �� 10 ����������� �����������.

## �����������
- **C++17**: ���������� � ���������� ��������� (��������, MSVC).
- **Boost**: ���������� ��� ������ � ����� (������ 1.87+).\
    ���������: �������� � [boost.org](https://www.boost.org/), ����� ��������:
```
bootstrap.bat
b2 --with-asio --with-beast --with-system --with-date_time -j4 link=static threading=multi runtime-link=shared address-model=64 stage
```
- **PostgreSQL**: ���� ������ (������ 13+).
- **libpqxx**: C++-���������� ��� PostgreSQL (������ 7.10+).
- **MyHTML**: ���������� �������� HTML (������ ��� SpiderApp).\
���������: 
1. �������� � [GitHub](https://github.com/lexborisov/myhtml) (������ 4.0.5+).
2. �������� `CMakeLists.txt` � ������� ������:
```
if(NOT MyHTML_BUILD_WITHOUT_THREADS)
```
�������� � ��:
```
if(NOT MyHTML_BUILD_WITHOUT_THREADS AND NOT WIN32)
```
   ��� ��������� ������ �� Windows ��� �������������.

3. ������:
```
mkdir build
cd build
cmake ..
cmake --build . --config Release
cmake --install .
```
- **OpenSSL**: ��� HTTPS-���������� � SpiderApp.

## ������ �������

1. �������� ���������� ��� ������:
```
mkdir build
cd build
```
2. ��������������� ������:
```
cmake -DBOOST_ROOT="path/to/boost-1.87.0" ^
      -DLIBPQXX_DIR="path/to/libpqxx-7.10.0" ^
      -DMYHTML_INSTALL_DIR="path/to/myhtml-4.0.5/build/install" ^ ..
```
- ������� ���� ���� � `BOOST_ROOT`, `LIBPQXX_DIR`, `MYHTML_INSTALL_DIR`. 
- **OpenSSL** ������ ���� �������� � ���������� ����� PATH.
3. �������� ������:
```
cmake --build . --config Release
```
� `build/Release` �������� `SpiderApp.exe` � `HttpServerApp.exe`.

## ���������

- **������������ ���� ������**:\
�������� ���� ������ PostgreSQL. ������� ��������� ������������� ��� ������� SpiderApp.
- **���� ������������**:\
������� ���� ������ ��� ����������� � �� � config.ini. ������� ������� � ��������� URL. ������:
```
[database]
host=localhost
port=5432
dbname=your_db
user=your_user
password=your_password

[spider]
depth=2
start_url=https://en.wikipedia.org/wiki/Main_Page

[search]
port=8081
```

## ������

### SpiderApp
������ � �������:
```
cd Realese
SpiderApp
```
���� ����� ������� � start_url �� ������� depth ��� ������ ���������� 10,000 URL.

### HttpServerApp:
����� ������ ����� ������ � �������:
```
HttpServerApp
```
���������� ������ �� ������� � �������� �� � ��������. ������� ������.

> [!NOTE]
> - ���������, ��� PostgreSQL �������.
> - ���� ����� ��������� � ������� (����� ������ � �� � URL).
> - ������ ��������� ������ ��������� �����, ����� � ������� � ��������.
> - ����������� � 10000 URL �����������, ����� �� ����������� �������.

## ������� ������
- ������ ������ **SpiderApp**
![������ ������ SpiderApp](images/spider_example.png)
- ������ ������ **HttpServerApp**
![������ ������ HttpServerApp](images/server_example_1.png)
![������ ������ HttpServerApp](images/server_example_2.png)
![������ ������ HttpServerApp](images/server_example_3.png)


## ��������
������ ���������������� ��� ��������� MIT (��. ���� [LICENSE](LICENSE)).