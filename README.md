### 开启ollama服务器
```
ollama  serve
```
### 下载deepseek模型
```
ollama run deepseek-r1:1.5b
```
### 进行交叉编译和清理build文件夹
```
make -j12
//-j12意味着开12条线程一起编译会更快根据电脑情况可加减数字
make clean
//修改了配置文件才需要，删掉编译生成的所有文件
```
### 将编译后的文件上传至开发板（WIFI传输），获取IP后通过SSH协议登录
```
scp main root@192.168.208.87 /root/
ssh root@192.168.208.87
```
### 主要功能页面展示
#### 开机动画
<img width="732" height="402" alt="image" src="https://github.com/user-attachments/assets/97baa506-fc42-467d-9986-0db98dde5d37" />

#### 登录页面
<img width="732" height="422" alt="image" src="https://github.com/user-attachments/assets/87c67d86-434e-4b94-b1d3-4180296e4392" />

#### 注册页面
<img width="733" height="400" alt="image" src="https://github.com/user-attachments/assets/a8e5ae30-a628-4d09-8443-35141c04c0ee" />

#### 忘记密码页面
<img width="732" height="442" alt="image" src="https://github.com/user-attachments/assets/d1f31dba-4e01-4935-bb91-e20b23e41a24" />

#### deepseek对话页面
<img width="732" height="416" alt="image" src="https://github.com/user-attachments/assets/22273ef5-ee2a-45ca-810c-4befa695c325" />

#### 双人打地鼠页面
<img width="732" height="395" alt="image" src="https://github.com/user-attachments/assets/97e89ac8-0825-42fe-8a7e-ec6d97af0d0e" />

#### 2048页面
<img width="731" height="413" alt="image" src="https://github.com/user-attachments/assets/11e61136-6d43-4aee-a082-f55bc4ae3b54" />
