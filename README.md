# 说明

程序演示了一个简单的粒子系统，捕获鼠标的动作，在鼠标滑过的线段内，均匀的产生粒子。
粒子的初始速度，由一个在圆球区域内随机产生速度的发射器完成。
开始速度比较小，以便模拟笔写留下字迹的效果，随后设置摩擦力为负值（正值减速，负值加速），模拟笔画扩散的效果。
粒子产生的位置，首先用随机生成的颜色进行绘制，然后贴上纹理，进行blending操作。
粒子的颜色值会随着时间的变化而不断变化。产生一种变换的效果。
粒子纹理了开启GL_POINT_SPRITE状态，所以需要openGL2.0以上的支持。

## 程序目录说明：

lib文件夹：spark编译后的静态库文件
external文件夹：需要使用的第三方库文件
res文件夹：用到的资源文件

## 使用的第三方库：
1）SPARK particle systme library http://spark.developpez.com/
2）SDL simple directmedia layer http://www.libsdl.org/

## 编程环境：
vc++2010	win7 64位
