#设计思路
1. 图形界面
	VT100控制码
2. 方块相关
	用4x4的矩阵来表示一个图案，存储方式用二维数组。
	{
	 0,0,0,0
	 0,0,1,1
	 1,1,0,0
	 0,0,0,0
	}
	{
	 0,1,0,0
	 1,1,1,0
	 0,0,0,0
	 0,0,0,0
	}
3. 方块操作
	———————————————>x
	|
	|
	|
	|
	y
	
	1)给定坐标原点，画出4x4的图案。非0画色块，0不画。
	2)给定坐标原点，擦除4x4的图案。
	3)顺时针旋转
	4)每次生成一个图案时，随机填充单个块的颜色
4. 使用一个大的矩阵来标识画布，存储方式为二维数组。画图边界不超过这个数组。
   图案浮在画布上移动，到不能移动时，填充入画布。上下到顶，则游戏结束。
5. 实时响应键盘，色块变换时，立刻重绘。
6. 每隔一段时间运动，和相应键盘的方案：
	1). 消息驱动。创建消息队列，分辨发送不同的消息。主进程中做出相应
	2). 在键盘相应中调用接口修改参数。
	第一种比较好，耦合性更小。
