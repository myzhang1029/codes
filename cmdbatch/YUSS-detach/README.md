# YUSS-detach
[中文介绍](#脱控魔术手)
## The origin
In the computer room of YUSS, the teacher uses a software called `StudentMain.exe`
to control the students' computers. It's developed by a company called Mythware.
## Our expedition
But as a teenager, many of us wanted a way to escape from the control.
One of the ways is to enter the password and uninstall the software.
We found the password is "lonben". But this way is hard to recover if the
teacher finds out. So I chose another way: to kill the process.  
## Making YUSS-detach
The process is special, we can't kill it using the `taskmgr` command or the
`taskkill` command. After a bit of research, I found that I needed to download a `ntsd.exe`.  
Then I made a simple bat script to automate that process, but then I found that
sometimes the system had two `StudentMain.exe` processes, one started by SYSTEM
and the other by Administrator. ntsd.exe doesn't know how to handle that,
so I added a `taskkill` before the `ntsd` so both processes would be killed.(2019.6.24)
After that, on June 28th, I changed `ntsd.exe` to `cdb.exe` because it does not
start a new window, so users won't get confused.
## The future
When you see this text, we have already finished our computer classes and
we're now heading towards the high school entrance exam. If you are our junior
ones, feel free to use this pack. Otherwise, it's just kept as a sweet memory
with you guys... finger heart...

# 脱控魔术手
## 缘起
在云大附中的机房里，老师用一个“极域电子教室”的“学生端程序”控制我们的电脑。
## 探索
我们肯定都想脱离老师控制，所以我们进行了探索。我们发现可以输入密码，然后卸载这个软件。
但是如果老师来了，这样你就完了。所以我决定探索另一条路：杀死进程。
## 制作
这个`StudentMain.exe`很特别，用任务管理器和`taskkill`命令都杀不死，百度发现，有个东西叫
`ntsd.exe`，它可以“干掉”学生端程序。
但是光光这样还不行。有些时候会有两个“学生端程序”，一个是SYSTEM用户启动的，另一个属于Administrator
这种情况`ntsd.exe` 就不行了。所以我在程序里加了一行`taskkill`，这样就可以分别杀掉两个进程。
然后，后来我又把`ntsd.exe`换成了`cdb.exe`，因为这样就不会启动一个新的窗口。
## 未来
现在，我们已经没有信息技术课了。我们所面对的是中考。假如你是我们的学弟，然后想上外网，插U盘，打游戏……
欢迎你使用这个软件。我们也把它上传到了考试酷班级248535，你也可以申请加入这个班级，这样哪怕老师
封了网络，你也可以下到这个软件。另外，据我对《信息技术教室管理**》的阅读，使用这个东西没有明显违反规定。  
当然，还有一年，我们也要毕业了。所以，这个东西更多的还是我与同学们深深的回忆……我戏太多，舍不得离开。
