# TODO List

* [x] Render Graph / Render Resource Reuse
--- 
* [X] 字体渲染 (Partial) (是否引入ICU?)
* [ ] 带引用计数的各项资产管理 (Partial Done)
* [x] 异步图像加载
* [x] Task Graph
* [ ] MSAA，或者其它效果接近MSAA的抗锯齿
* [ ] Mesh Shader支持的粒子特效
* [ ] 层后处理
* [x] 更好的Bloom
* [ ] 投射阴影
* [ ] 更好的SSAO
* [ ] Modern UI / WIP
* [ ] 序列化 / WIP
* [ ] Mesh Shader的批处理，把texture indices转换到图元属性上
* [x] 直接生成而不是通过blit获取mipmap
---
* 文本元素的布局

---

# 绘制
* [ ] 使用alpha通道作为sdf的可选限界？
* [ ] 考虑为sdf提供多颜色支持？或者在alpha通道绑定多个信息？
* [ ] 将sdf的rgba8直接打包进f32?
* [ ] 绘制实心多边形的顶点优化
* [ ] 使用sdf圆贴图来替代整单色圆绘制，使得顶点减少为常数级并且自带抗锯齿

* [x] UI三层式绘制：背景色，正常色，发光色
* [x] UI各总层的局部blit
* [ ] UI各总层的局部blit在嵌套视图中的位置修正

---

# ECS

* [x] 对各slice的defer action的stage, dependency, read, write的标记
* [x] 对各slice的task graph
* [ ] 并行化前碰撞检测 / WIP
* [ ] 更高效的Quad Tree插入？

# Entity
* [ ] 子网格 / WIP

# Editor
* [x] 碰撞箱编辑器 (Partial) 
* [ ] 子网格形状编辑器
* [ ] 子网格建筑编辑器

# Move Command
* [ ] 路径点编辑器
* [ ] 路径旋转编辑器
* [ ] 参考系编辑器
* [ ] Undo / Redo
* 