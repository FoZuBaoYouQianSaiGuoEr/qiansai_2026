from libs.PipeLine import PipeLine
from libs.AIBase import AIBase
from libs.AI2D import Ai2d
from libs.Utils import *
import os,sys,ujson,gc,math,time
from media.media import *
import nncase_runtime as nn
import ulab.numpy as np
import image
import aidemo

# ============ 蜂鸣器开关 ============
BUZZER_ENABLE = True  # 设为 False 关闭蜂鸣器

# ============ 蜂鸣器配置 ============
BUZZER_GPIO = 43   # 庐山派板载蜂鸣器 GPIO43
BUZZER_PWM  = 1    # PWM通道1

try:
    from machine import PWM, FPIOA
    _fpioa = FPIOA()
    _fpioa.set_function(BUZZER_GPIO, FPIOA.PWM1 if BUZZER_PWM == 1 else FPIOA.PWM0)
    buzzer = PWM(BUZZER_PWM, freq=2700, duty=0)
    BUZZER_OK = True
    buzzer.duty(50); time.sleep_ms(80); buzzer.duty(0)
    print("[Buzzer] OK - GPIO{} -> PWM{}".format(BUZZER_GPIO, BUZZER_PWM))
except Exception as e:
    BUZZER_OK = False
    buzzer = None
    print("[Buzzer] FAIL: {}".format(e))

# 蜂鸣器状态机
_buzz_last_ts = 0
_buzzing = False
_buzz_start = 0
_buzz_duration = 0

def buzz_tick(alert_active):
    """非阻塞蜂鸣, 每帧调用"""
    global BUZZER_OK, buzzer, _buzz_last_ts, _buzzing, _buzz_start, _buzz_duration
    if not BUZZER_ENABLE or not BUZZER_OK or buzzer is None:
        return
    now = time.ticks_ms()
    if not alert_active:
        if _buzzing:
            try:
                buzzer.duty(0)
            except Exception:
                pass
            _buzzing = False
        return
    if _buzzing:
        if time.ticks_diff(now, _buzz_start) >= _buzz_duration:
            try:
                buzzer.duty(0)
            except Exception:
                pass
            _buzzing = False
    if not _buzzing:
        if time.ticks_diff(now, _buzz_last_ts) >= 600:
            _buzz_last_ts = now
            _buzz_start = now
            _buzz_duration = 120  # ms
            _buzzing = True
            try:
                buzzer.freq(2700)
                buzzer.duty(60)
            except Exception:
                _buzzing = False

# ============ GPIO告警输出 ============
# GPIO42 → 人脸检测指示 (高电平有效)
try:
    from machine import Pin
    _gpio42 = Pin(42, Pin.OUT, value=0)
    GPIO_OK = True
    print("[GPIO] OK - GPIO42(face_detected)")
except Exception as e:
    GPIO_OK = False
    _gpio42 = None
    print("[GPIO] FAIL: {}".format(e))

# ============ 陌生人检测 GPIO44 ============
try:
    _gpio44 = Pin(44, Pin.OUT, value=0)
    STRANGER_GPIO_OK = True
    print("[GPIO44] OK - stranger alert")
except Exception as e:
    STRANGER_GPIO_OK = False
    _gpio44 = None
    print("[GPIO44] FAIL: {}".format(e))

# 陌生人检测参数
REGISTER_TIME_MS = 5000       # 注册阶段5秒
STRANGER_PERSIST_FRAMES = 90  # 持续90帧(3秒)才告警
_register_max_faces = 0
_stranger_frames = 0
_stranger_active = False
_start_ts = time.ticks_ms()

# 自定义人脸检测类，继承自AIBase基类
class FaceDetectionApp(AIBase):
    def __init__(self, kmodel_path, model_input_size, anchors, confidence_threshold=0.5, nms_threshold=0.2, rgb888p_size=[224,224], display_size=[1920,1080], debug_mode=0):
        super().__init__(kmodel_path, model_input_size, rgb888p_size, debug_mode)  # 调用基类的构造函数
        self.kmodel_path = kmodel_path  # 模型文件路径
        self.model_input_size = model_input_size  # 模型输入分辨率
        self.confidence_threshold = confidence_threshold  # 置信度阈值
        self.nms_threshold = nms_threshold  # NMS（非极大值抑制）阈值
        self.anchors = anchors  # 锚点数据，用于目标检测
        self.rgb888p_size = [ALIGN_UP(rgb888p_size[0], 16), rgb888p_size[1]]  # sensor给到AI的图像分辨率，并对宽度进行16的对齐
        self.display_size = [ALIGN_UP(display_size[0], 16), display_size[1]]  # 显示分辨率，并对宽度进行16的对齐
        self.debug_mode = debug_mode  # 是否开启调试模式
        self.ai2d = Ai2d(debug_mode)  # 实例化Ai2d，用于实现模型预处理
        self.ai2d.set_ai2d_dtype(nn.ai2d_format.NCHW_FMT, nn.ai2d_format.NCHW_FMT, np.uint8, np.uint8)  # 设置Ai2d的输入输出格式和类型

    # 配置预处理操作，这里使用了pad和resize，Ai2d支持crop/shift/pad/resize/affine，具体代码请打开/sdcard/app/libs/AI2D.py查看
    def config_preprocess(self, input_image_size=None):
        with ScopedTiming("set preprocess config", self.debug_mode > 0):  # 计时器，如果debug_mode大于0则开启
            ai2d_input_size = input_image_size if input_image_size else self.rgb888p_size  # 初始化ai2d预处理配置，默认为sensor给到AI的尺寸，可以通过设置input_image_size自行修改输入尺寸
            top, bottom, left, right,_ =letterbox_pad_param(self.rgb888p_size,self.model_input_size) 
            self.ai2d.pad([0, 0, 0, 0, top, bottom, left, right], 0, [104, 117, 123])  # 填充边缘
            self.ai2d.resize(nn.interp_method.tf_bilinear, nn.interp_mode.half_pixel)  # 缩放图像
            self.ai2d.build([1,3,ai2d_input_size[1],ai2d_input_size[0]],[1,3,self.model_input_size[1],self.model_input_size[0]])  # 构建预处理流程

    # 自定义当前任务的后处理，results是模型输出array列表，这里使用了aidemo库的face_det_post_process接口
    def postprocess(self, results):
        with ScopedTiming("postprocess", self.debug_mode > 0):
            post_ret = aidemo.face_det_post_process(self.confidence_threshold, self.nms_threshold, self.model_input_size[1], self.anchors, self.rgb888p_size, results)
            if len(post_ret) == 0:
                return post_ret
            else:
                return post_ret[0]

    # 绘制检测结果到画面上
    def draw_result(self, pl, dets):
        with ScopedTiming("display_draw", self.debug_mode > 0):
            global _stranger_active
            if dets:
                pl.osd_img.clear()  # 清除OSD图像
                for det in dets:
                    # 将检测框的坐标转换为显示分辨率下的坐标
                    x, y, w, h = map(lambda x: int(round(x, 0)), det[:4])
                    x = x * self.display_size[0] // self.rgb888p_size[0]
                    y = y * self.display_size[1] // self.rgb888p_size[1]
                    w = w * self.display_size[0] // self.rgb888p_size[0]
                    h = h * self.display_size[1] // self.rgb888p_size[1]
                    pl.osd_img.draw_rectangle(x, y, w, h, color=(255, 255, 0, 255), thickness=2)  # 绘制矩形框
                # 检测到人脸 → 蜂鸣 + GPIO42高电平
                buzz_tick(True)
                try:
                    if BUZZER_ENABLE and GPIO_OK and _gpio42 is not None:
                        _gpio42.value(1)
                except Exception:
                    pass
                # 陌生人告警 → GPIO44
                if _stranger_active:
                    try:
                        if STRANGER_GPIO_OK and _gpio44 is not None:
                            _gpio44.value(1)
                    except Exception:
                        pass
            else:
                pl.osd_img.clear()
                # 无人脸 → 静音 + GPIO42低电平 + GPIO44低电平
                buzz_tick(False)
                try:
                    if GPIO_OK and _gpio42 is not None:
                        _gpio42.value(0)
                except Exception:
                    pass
                try:
                    if STRANGER_GPIO_OK and _gpio44 is not None:
                        _gpio44.value(0)
                except Exception:
                    pass

if __name__ == "__main__":
    # 添加显示模式，默认hdmi，可选hdmi/lcd/lt9611/st7701/hx8399/nt35516,其中hdmi默认置为lt9611，分辨率1920*1080；lcd默认置为st7701，分辨率800*480
    display_mode="hdmi"
    # k230保持不变，k230d可调整为[640,360]
    rgb888p_size = [1280, 720]
    # 设置模型路径和其他参数
    kmodel_path = "/sdcard/examples/kmodel/face_detection_320.kmodel"
    # 其它参数
    confidence_threshold = 0.5
    nms_threshold = 0.2
    anchor_len = 4200
    det_dim = 4
    anchors_path = "/sdcard/examples/utils/prior_data_320.bin"
    anchors = np.fromfile(anchors_path, dtype=np.float)
    anchors = anchors.reshape((anchor_len, det_dim))

    # 初始化PipeLine，用于图像处理流程
    pl = PipeLine(rgb888p_size=rgb888p_size, display_mode=display_mode)
    pl.create()  # 创建PipeLine实例
    display_size=pl.get_display_size()
    # 初始化自定义人脸检测实例
    face_det = FaceDetectionApp(kmodel_path, model_input_size=[320, 320], anchors=anchors, confidence_threshold=confidence_threshold, nms_threshold=nms_threshold, rgb888p_size=rgb888p_size, display_size=display_size, debug_mode=0)
    face_det.config_preprocess()  # 配置预处理
    while True:
        with ScopedTiming("total",1):
            img = pl.get_frame()            # 获取当前帧数据
            res = face_det.run(img)         # 推理当前帧

            # ===== 陌生人检测 =====
            now_ts = time.ticks_ms()
            face_count = len(res[0]) if res else 0
            if time.ticks_diff(now_ts, _start_ts) < REGISTER_TIME_MS:
                if face_count > _register_max_faces:
                    _register_max_faces = face_count
            else:
                if face_count > _register_max_faces:
                    _stranger_frames += 1
                    if _stranger_frames >= STRANGER_PERSIST_FRAMES:
                        _stranger_active = True
                else:
                    _stranger_frames = max(0, _stranger_frames - 2)
                    if _stranger_frames == 0:
                        _stranger_active = False

            face_det.draw_result(pl, res)   # 绘制结果
            pl.show_image()                 # 显示结果
            gc.collect()                    # 垃圾回收
    face_det.deinit()                       # 反初始化
    pl.destroy()                            # 销毁PipeLine实例

