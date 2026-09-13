# M1 Stock Scheduler Workload Characterization

日期：2026-09-13
硬件：STM32U5A9J-DK
GUI：LVGL 9.3.0
配置：SW + STM32 DMA2D + NemaGFX，`LV_USE_PROFILER = 0`，`LV_DRAW_SCHED_TRACE = 1`

## 数据有效性

- 16 个 benchmark scene 全部存在，CSV 共 197 个聚合项。
- `pixel_count` 全部为有效十进制数。
- 所有记录均通过对应 area bucket 的像素上下界校验。
- `area_bucket=0` 的记录全部满足 `pixel_count=0`。
- 本文只保留修复 64 位输出后的最新数据，不包含上一轮 `pixel_count=lu` 的无效输出。

## 分析报告

### 总体 workload

本次共执行 68,058 个 Draw Task，累计有效包围盒面积为 924,990,011 pixels。

| Renderer | Task 数 | Task 占比 | Pixel 数 | Pixel 占比 | 平均 pixels/task |
|---|---:|---:|---:|---:|---:|
| DMA2D | 15,305 | 22.49% | 502,795,528 | 54.36% | 32,851.7 |
| NEMA | 48,339 | 71.03% | 388,235,071 | 41.97% | 8,031.5 |
| SW | 4,414 | 6.49% | 33,959,412 | 3.67% | 7,693.6 |

NEMA 接收最多任务，DMA2D 处理最多像素，SW 主要承担少数复杂 primitive。

### Primitive 分布

| Task type | Task 数 | Task 占比 | Pixel 数 | Pixel 占比 | 平均 pixels/task |
|---|---:|---:|---:|---:|---:|
| FILL | 23,040 | 33.85% | 614,408,929 | 66.42% | 26,667.1 |
| BORDER | 3,323 | 4.88% | 120,267,126 | 13.00% | 36,192.3 |
| LABEL | 20,644 | 30.33% | 81,394,027 | 8.80% | 3,942.7 |
| IMAGE | 3,169 | 4.66% | 71,036,902 | 7.68% | 22,416.2 |
| ARC | 1,981 | 2.91% | 26,548,682 | 2.87% | 13,401.7 |
| BOX_SHADOW | 1,842 | 2.71% | 5,830,970 | 0.63% | 3,165.6 |
| LAYER | 368 | 0.54% | 2,980,800 | 0.32% | 8,100.0 |
| LINE | 13,397 | 19.68% | 2,272,416 | 0.25% | 169.6 |
| TRIANGLE | 294 | 0.43% | 250,159 | 0.03% | 850.9 |

像素吞吐重点是 FILL、BORDER、IMAGE 和 ARC；task 启动与调度开销重点是 LABEL 和 LINE。

### Area bucket 分布

| Area bucket | Task 数 | Task 占比 | Pixel 数 | Pixel 占比 |
|---|---:|---:|---:|---:|
| 0 | 9,123 | 13.40% | 0 | 0.00% |
| 1-64 | 3,581 | 5.26% | 177,656 | 0.02% |
| 65-256 | 4,838 | 7.11% | 784,927 | 0.08% |
| 257-1024 | 10,063 | 14.79% | 5,872,983 | 0.63% |
| 1025-4096 | 18,351 | 26.96% | 52,243,160 | 5.65% |
| 4097-16384 | 13,544 | 19.90% | 128,667,233 | 13.91% |
| 16385-65536 | 5,366 | 7.88% | 170,052,684 | 18.38% |
| >65536 | 3,192 | 4.69% | 567,191,368 | 61.32% |

仅 4.69% 的超大 task 贡献 61.32% 的像素。另一方面，不超过 256 pixels 或零面积的 task 共 17,542 个，贡献像素极少，但可能产生显著固定调度与提交开销。

### 实际路由

| Task type | Renderer | Task 数 | Pixel 数 | 总 Pixel 占比 |
|---|---|---:|---:|---:|
| FILL | DMA2D | 12,553 | 483,222,008 | 52.24% |
| FILL | NEMA | 10,193 | 129,878,920 | 14.04% |
| FILL | SW | 294 | 1,308,001 | 0.14% |
| BORDER | NEMA | 3,320 | 120,245,526 | 13.00% |
| BORDER | SW | 3 | 21,600 | <0.01% |
| LABEL | NEMA | 20,644 | 81,394,027 | 8.80% |
| IMAGE | NEMA | 417 | 51,463,382 | 5.56% |
| IMAGE | DMA2D | 2,752 | 19,573,520 | 2.12% |
| ARC | SW | 1,981 | 26,548,682 | 2.87% |
| BOX_SHADOW | SW | 1,842 | 5,830,970 | 0.63% |
| LAYER | NEMA | 368 | 2,980,800 | 0.32% |
| LINE | NEMA | 13,397 | 2,272,416 | 0.25% |
| TRIANGLE | SW | 294 | 250,159 | 0.03% |

- DMA2D 主要执行普通 FILL 和未变换 IMAGE。
- NEMA 执行 LABEL、LINE、BORDER、LAYER、旋转 IMAGE 和较复杂 FILL。
- SW 主要执行 ARC、BOX_SHADOW 和 TRIANGLE。
- Rotated ARGB images 的 IMAGE 全部路由到 NEMA；普通 RGB/ARGB images 全部路由到 DMA2D。
- FILL 在 Containers 系列由 DMA2D/NEMA 分担，在 Widgets demo 中三个 renderer 都有实际执行。
- 当前数据表示 task type/size 层面的实际路由重叠，不表示单个 task 的完整 renderer eligibility mask。

### 重点 scene

| Scene | Task 数 | Pixel 数 | Pixel 占比 | Benchmark |
|---|---:|---:|---:|---|
| Widgets demo | 35,149 | 282,864,884 | 30.58% | 99% CPU / 21 FPS |
| Containers with overlay | 5,016 | 152,319,382 | 16.47% | 73% CPU / 77 FPS |
| Containers with scrolling | 8,183 | 133,199,647 | 14.40% | 68% CPU / 50 FPS |
| Moving wallpaper | 1,310 | 108,692,296 | 11.75% | 48% CPU / 79 FPS |
| Screen sized text | 846 | 69,350,016 | 7.50% | 78% CPU / 29 FPS |

前五个 scene 合计贡献约 80.7% pixels。Widgets demo 同时贡献 51.65% 的全部 task，是研究小 task 固定成本的首要 workload；overlay 和 scrolling 更适合研究大面积 DMA2D/NEMA workload。

### 零面积 task

共 9,123 个 task 的 `task->area ∩ task->clip_area` 为空，占全部 task 的 13.40%。主要来源：

- Widgets demo LINE/NEMA：5,526
- Widgets demo LABEL/NEMA：2,730
- Widgets demo FILL/NEMA：397
- Multiple labels FILL/DMA2D：235

这些 task 已被实际 Draw Unit 接受，但不产生有效包围盒像素。M2 不应把它们放入像素吞吐测试，可以单独研究 evaluate、dispatch 和空裁剪 task 的固定成本。

### M2 建议

1. DMA2D/SW/NEMA simple fill：512、2K、8K、32K、约 100K、230,400 pixels，并区分 opaque/alpha。
2. NEMA fill/border：2-4K、8-16K、32K、约 100K pixels。
3. IMAGE：普通 RGB/ARGB 选择约 2.8K、8-10K、22K；rotated/transformed 选择约 10K；另测 230,400 pixels full-screen。
4. LABEL：约 180、600、2.6K、9.8K、约 222K pixels。
5. LINE：重点测固定启动成本，代表尺寸选择 32、180、540、1.8K pixels。
6. SW ARC：约 474、3K、10K、29K pixels。

`pixel_count` 是裁剪后包围盒面积，不是曲线、边框或文字真正写入的像素数。它适合表示 task 尺寸和调度模型输入，不应直接当作所有 primitive 的实际 memory-write 数量。另外，各 scene 运行时间固定，因此更快的 scene 会生成更多帧和更多 task；跨 scene 比较总量时应结合 FPS，不能把总 task 数直接视为单帧 workload。

## 最新原始数据

```csv
scene_id,scene_name,task_type,renderer,area_bucket,task_count,pixel_count
0,Empty screen,FILL,DMA2D,4097-16384,472,4716320
0,Empty screen,FILL,DMA2D,>65536,236,54374400
0,Empty screen,LABEL,NEMA,1025-4096,188,755720
0,Empty screen,LABEL,NEMA,4097-16384,284,3824880
1,Moving wallpaper,FILL,DMA2D,4097-16384,437,4396094
1,Moving wallpaper,FILL,DMA2D,16385-65536,1,17280
1,Moving wallpaper,FILL,DMA2D,>65536,217,49996800
1,Moving wallpaper,LABEL,NEMA,1025-4096,46,186760
1,Moving wallpaper,LABEL,NEMA,4097-16384,390,4075780
1,Moving wallpaper,IMAGE,NEMA,4097-16384,1,5502
1,Moving wallpaper,IMAGE,NEMA,16385-65536,1,17280
1,Moving wallpaper,IMAGE,NEMA,>65536,217,49996800
2,Single rectangle,FILL,DMA2D,4097-16384,32,272340
2,Single rectangle,FILL,DMA2D,16385-65536,246,4338908
2,Single rectangle,FILL,DMA2D,>65536,1,230400
2,Single rectangle,LABEL,NEMA,1025-4096,10,39935
2,Single rectangle,LABEL,NEMA,4097-16384,12,173230
3,Multiple rectangles,FILL,DMA2D,257-1024,245,140064
3,Multiple rectangles,FILL,DMA2D,4097-16384,2156,26392598
3,Multiple rectangles,FILL,DMA2D,16385-65536,10,172800
3,Multiple rectangles,FILL,DMA2D,>65536,1,230400
3,Multiple rectangles,LABEL,NEMA,65-256,5,1245
3,Multiple rectangles,LABEL,NEMA,257-1024,230,61812
3,Multiple rectangles,LABEL,NEMA,1025-4096,3,12110
3,Multiple rectangles,LABEL,NEMA,4097-16384,19,202665
4,Multiple RGB images,FILL,DMA2D,4097-16384,458,4578166
4,Multiple RGB images,FILL,DMA2D,16385-65536,10,172800
4,Multiple RGB images,FILL,DMA2D,>65536,1,230400
4,Multiple RGB images,LABEL,NEMA,1025-4096,3,12215
4,Multiple RGB images,LABEL,NEMA,4097-16384,19,202490
4,Multiple RGB images,IMAGE,DMA2D,4097-16384,430,4300000
5,Multiple ARGB images,FILL,DMA2D,4097-16384,458,4578360
5,Multiple ARGB images,FILL,DMA2D,16385-65536,10,172800
5,Multiple ARGB images,FILL,DMA2D,>65536,1,230400
5,Multiple ARGB images,LABEL,NEMA,1025-4096,2,8120
5,Multiple ARGB images,LABEL,NEMA,4097-16384,20,206725
5,Multiple ARGB images,IMAGE,DMA2D,4097-16384,430,4300000
6,Rotated ARGB images,FILL,DMA2D,4097-16384,32,273878
6,Rotated ARGB images,FILL,DMA2D,16385-65536,150,3015640
6,Rotated ARGB images,FILL,DMA2D,>65536,1,230400
6,Rotated ARGB images,LABEL,NEMA,1025-4096,8,32585
6,Rotated ARGB images,LABEL,NEMA,4097-16384,14,181455
6,Rotated ARGB images,IMAGE,NEMA,4097-16384,141,1410000
7,Multiple labels,FILL,DMA2D,0,235,0
7,Multiple labels,FILL,DMA2D,1025-4096,2820,11280000
7,Multiple labels,FILL,DMA2D,4097-16384,32,276446
7,Multiple labels,FILL,DMA2D,16385-65536,10,172800
7,Multiple labels,FILL,DMA2D,>65536,1,230400
7,Multiple labels,LABEL,NEMA,1025-4096,2834,7173115
7,Multiple labels,LABEL,NEMA,4097-16384,20,206935
8,Screen sized text,FILL,DMA2D,4097-16384,282,2836764
8,Screen sized text,FILL,DMA2D,>65536,141,32486400
8,Screen sized text,LABEL,NEMA,1025-4096,11,44660
8,Screen sized text,LABEL,NEMA,4097-16384,271,2709520
8,Screen sized text,LABEL,NEMA,>65536,141,31272672
9,Multiple arcs,FILL,DMA2D,257-1024,573,298268
9,Multiple arcs,FILL,DMA2D,4097-16384,32,276076
9,Multiple arcs,FILL,DMA2D,16385-65536,10,172800
9,Multiple arcs,FILL,DMA2D,>65536,1,230400
9,Multiple arcs,LABEL,NEMA,4097-16384,22,215020
9,Multiple arcs,ARC,SW,257-1024,573,264242
10,Containers,FILL,DMA2D,4097-16384,32,275284
10,Containers,FILL,DMA2D,16385-65536,100,3113100
10,Containers,FILL,DMA2D,>65536,1,230400
10,Containers,FILL,NEMA,1025-4096,92,311328
10,Containers,FILL,NEMA,16385-65536,92,2909408
10,Containers,BORDER,NEMA,16385-65536,92,2980800
10,Containers,BOX_SHADOW,SW,1025-4096,92,311328
10,Containers,LABEL,NEMA,257-1024,92,91264
10,Containers,LABEL,NEMA,1025-4096,186,527552
10,Containers,LABEL,NEMA,4097-16384,20,206480
10,Containers,IMAGE,DMA2D,4097-16384,92,588800
11,Containers with overlay,FILL,DMA2D,4097-16384,456,4581812
11,Containers with overlay,FILL,DMA2D,>65536,456,105062400
11,Containers with overlay,FILL,NEMA,1025-4096,456,1543104
11,Containers with overlay,FILL,NEMA,16385-65536,456,14420544
11,Containers with overlay,BORDER,NEMA,16385-65536,456,14774400
11,Containers with overlay,BOX_SHADOW,SW,1025-4096,456,1543104
11,Containers with overlay,LABEL,NEMA,257-1024,456,452352
11,Containers with overlay,LABEL,NEMA,1025-4096,925,2627356
11,Containers with overlay,LABEL,NEMA,4097-16384,443,4395910
11,Containers with overlay,IMAGE,DMA2D,4097-16384,456,2918400
12,Containers with opa,FILL,DMA2D,4097-16384,32,275524
12,Containers with opa,FILL,DMA2D,16385-65536,100,3113100
12,Containers with opa,FILL,DMA2D,>65536,1,230400
12,Containers with opa,FILL,NEMA,1025-4096,92,311328
12,Containers with opa,FILL,NEMA,16385-65536,92,2980800
12,Containers with opa,BORDER,NEMA,16385-65536,92,2980800
12,Containers with opa,BOX_SHADOW,SW,1025-4096,92,311328
12,Containers with opa,LABEL,NEMA,257-1024,92,91264
12,Containers with opa,LABEL,NEMA,1025-4096,186,527587
12,Containers with opa,LABEL,NEMA,4097-16384,20,206550
12,Containers with opa,IMAGE,DMA2D,4097-16384,92,588800
13,Containers with opa_layer,FILL,DMA2D,4097-16384,32,276118
13,Containers with opa_layer,FILL,DMA2D,16385-65536,100,3113100
13,Containers with opa_layer,FILL,DMA2D,>65536,1,230400
13,Containers with opa_layer,FILL,NEMA,257-1024,92,60536
13,Containers with opa_layer,FILL,NEMA,1025-4096,184,423384
13,Containers with opa_layer,FILL,NEMA,4097-16384,276,2736816
13,Containers with opa_layer,BORDER,NEMA,1025-4096,92,198720
13,Containers with opa_layer,BORDER,NEMA,4097-16384,276,2782080
13,Containers with opa_layer,BOX_SHADOW,SW,257-1024,92,60536
13,Containers with opa_layer,BOX_SHADOW,SW,1025-4096,92,250792
13,Containers with opa_layer,LABEL,NEMA,257-1024,92,91264
13,Containers with opa_layer,LABEL,NEMA,1025-4096,276,519432
13,Containers with opa_layer,LABEL,NEMA,4097-16384,22,215020
13,Containers with opa_layer,IMAGE,DMA2D,65-256,92,14720
13,Containers with opa_layer,IMAGE,DMA2D,1025-4096,184,574080
13,Containers with opa_layer,LAYER,NEMA,1025-4096,92,198720
13,Containers with opa_layer,LAYER,NEMA,4097-16384,276,2782080
14,Containers with scrolling,FILL,DMA2D,4097-16384,500,5025486
14,Containers with scrolling,FILL,DMA2D,>65536,250,57600000
14,Containers with scrolling,FILL,NEMA,0,17,0
14,Containers with scrolling,FILL,NEMA,65-256,7,1034
14,Containers with scrolling,FILL,NEMA,257-1024,34,20246
14,Containers with scrolling,FILL,NEMA,1025-4096,848,2757874
14,Containers with scrolling,FILL,NEMA,4097-16384,126,1259600
14,Containers with scrolling,FILL,NEMA,16385-65536,774,23395596
14,Containers with scrolling,BORDER,NEMA,257-1024,19,10260
14,Containers with scrolling,BORDER,NEMA,1025-4096,51,136620
14,Containers with scrolling,BORDER,NEMA,4097-16384,127,1270080
14,Containers with scrolling,BORDER,NEMA,16385-65536,777,23997870
14,Containers with scrolling,BOX_SHADOW,SW,0,10,0
14,Containers with scrolling,BOX_SHADOW,SW,65-256,7,1034
14,Containers with scrolling,BOX_SHADOW,SW,257-1024,18,11938
14,Containers with scrolling,BOX_SHADOW,SW,1025-4096,797,2623070
14,Containers with scrolling,LABEL,NEMA,1-64,2,124
14,Containers with scrolling,LABEL,NEMA,65-256,20,4064
14,Containers with scrolling,LABEL,NEMA,257-1024,824,793024
14,Containers with scrolling,LABEL,NEMA,1025-4096,1657,4701932
14,Containers with scrolling,LABEL,NEMA,4097-16384,429,4589875
14,Containers with scrolling,IMAGE,DMA2D,65-256,9,1360
14,Containers with scrolling,IMAGE,DMA2D,257-1024,25,14640
14,Containers with scrolling,IMAGE,DMA2D,1025-4096,105,260080
14,Containers with scrolling,IMAGE,DMA2D,4097-16384,750,4723840
15,Widgets demo,FILL,SW,1-64,1,33
15,Widgets demo,FILL,SW,65-256,6,466
15,Widgets demo,FILL,SW,257-1024,28,18174
15,Widgets demo,FILL,SW,1025-4096,135,361673
15,Widgets demo,FILL,SW,4097-16384,124,927655
15,Widgets demo,FILL,DMA2D,257-1024,2,744
15,Widgets demo,FILL,DMA2D,1025-4096,6,16740
15,Widgets demo,FILL,DMA2D,4097-16384,514,3326506
15,Widgets demo,FILL,DMA2D,16385-65536,144,4338492
15,Widgets demo,FILL,DMA2D,>65536,514,85390800
15,Widgets demo,FILL,NEMA,0,397,0
15,Widgets demo,FILL,NEMA,1-64,159,6924
15,Widgets demo,FILL,NEMA,65-256,850,142274
15,Widgets demo,FILL,NEMA,257-1024,1297,849762
15,Widgets demo,FILL,NEMA,1025-4096,2419,5060038
15,Widgets demo,FILL,NEMA,4097-16384,505,3235666
15,Widgets demo,FILL,NEMA,16385-65536,427,18619114
15,Widgets demo,FILL,NEMA,>65536,501,48833544
15,Widgets demo,BORDER,SW,4097-16384,3,21600
15,Widgets demo,BORDER,NEMA,65-256,1,180
15,Widgets demo,BORDER,NEMA,257-1024,11,7904
15,Widgets demo,BORDER,NEMA,1025-4096,38,101684
15,Widgets demo,BORDER,NEMA,4097-16384,358,2730704
15,Widgets demo,BORDER,NEMA,16385-65536,421,18399872
15,Widgets demo,BORDER,NEMA,>65536,509,49873552
15,Widgets demo,BOX_SHADOW,SW,0,4,0
15,Widgets demo,BOX_SHADOW,SW,65-256,2,480
15,Widgets demo,BOX_SHADOW,SW,257-1024,6,4320
15,Widgets demo,BOX_SHADOW,SW,1025-4096,24,65040
15,Widgets demo,BOX_SHADOW,SW,4097-16384,150,648000
15,Widgets demo,LABEL,NEMA,0,2730,0
15,Widgets demo,LABEL,NEMA,1-64,73,2447
15,Widgets demo,LABEL,NEMA,65-256,1903,342299
15,Widgets demo,LABEL,NEMA,257-1024,2809,1176991
15,Widgets demo,LABEL,NEMA,1025-4096,2326,5233486
15,Widgets demo,LABEL,NEMA,4097-16384,509,2998105
15,Widgets demo,IMAGE,DMA2D,65-256,1,160
15,Widgets demo,IMAGE,DMA2D,257-1024,3,2240
15,Widgets demo,IMAGE,DMA2D,1025-4096,8,21760
15,Widgets demo,IMAGE,DMA2D,4097-16384,34,347680
15,Widgets demo,IMAGE,DMA2D,16385-65536,41,916960
15,Widgets demo,IMAGE,NEMA,0,19,0
15,Widgets demo,IMAGE,NEMA,257-1024,38,33800
15,Widgets demo,LINE,NEMA,0,5526,0
15,Widgets demo,LINE,NEMA,1-64,3343,167975
15,Widgets demo,LINE,NEMA,65-256,1920,273254
15,Widgets demo,LINE,NEMA,257-1024,2333,1260944
15,Widgets demo,LINE,NEMA,1025-4096,255,455649
15,Widgets demo,LINE,NEMA,4097-16384,20,114594
15,Widgets demo,ARC,SW,0,27,0
15,Widgets demo,ARC,SW,1-64,2,120
15,Widgets demo,ARC,SW,65-256,9,1560
15,Widgets demo,ARC,SW,257-1024,30,21560
15,Widgets demo,ARC,SW,1025-4096,199,607950
15,Widgets demo,ARC,SW,4097-16384,387,3907630
15,Widgets demo,ARC,SW,16385-65536,754,21745620
15,Widgets demo,TRIANGLE,SW,0,158,0
15,Widgets demo,TRIANGLE,SW,1-64,1,33
15,Widgets demo,TRIANGLE,SW,65-256,6,797
15,Widgets demo,TRIANGLE,SW,257-1024,49,34834
15,Widgets demo,TRIANGLE,SW,1025-4096,61,115201
15,Widgets demo,TRIANGLE,SW,4097-16384,19,99294
```

### 官方 benchmark summary

```csv
Name,Avg. CPU,Avg. FPS,Avg. time,render time,flush time
Empty screen,26%,72,10,3,7
Moving wallpaper,48%,79,9,6,3
Single rectangle,10%,79,9,0,9
Multiple rectangles,40%,78,9,3,6
Multiple RGB images,21%,82,1,0,1
Multiple ARGB images,21%,82,3,1,2
Rotated ARGB images,18%,82,1,1,0
Multiple labels,79%,78,10,8,2
Screen sized text,78%,29,32,26,6
Multiple arcs,27%,81,6,1,5
Containers,22%,82,4,3,1
Containers with overlay,73%,77,10,10,0
Containers with opa,24%,82,4,3,1
Containers with opa_layer,31%,81,8,6,2
Containers with scrolling,68%,50,17,11,6
Widgets demo,99%,21,30,30,0
All scenes avg.,42%,70,10,7,3
```
