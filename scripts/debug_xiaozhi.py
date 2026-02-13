import websocket
import requests
import json
import time
import hashlib
import base64
import threading
import sys
import uuid

# ================= 配置区域 =================
# 🔴 请替换为你的实际参数
PRODUCT_ID = "04ec8e3f-caa6-458c-9dc1-1d6a179253bf"
PRODUCT_SECRET = "ba7c2987-cb69-4caf-b7bc-4ebd57b5a304"
# 模拟一个 Device ID，或者使用你真实的
DEVICE_ID = "1638a7ba" + str(uuid.uuid4())[:8]

# 接口地址 (参考 AItalk.py)
AUTH_URL = "https://api.listenai.com/v1/auth/tokens"
WS_URL = "wss://api.listenai.com/v1/interaction"
# ===========================================

class Colors:
    HEADER = '\033[95m'
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    GREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'

class XiaolingClient:
    def __init__(self):
        self.token = None
        self.ws = None
        self.is_connected = False

    def get_token(self):
        """
        第一步：获取授权 Token
        """
        print(f"{Colors.HEADER}=== 1. 开始设备端鉴权 ==={Colors.ENDC}")
        
        curtime = int(time.time())
        # 签名算法: md5(secret + curtime)
        data_to_encode = PRODUCT_SECRET + str(curtime)
        md5_hash = hashlib.md5()
        md5_hash.update(data_to_encode.encode('utf-8'))
        checksum = md5_hash.hexdigest()

        headers = {"Content-Type": "application/json"}
        payload = {
            "productId": PRODUCT_ID,
            "deviceId": DEVICE_ID,
            "curtime": curtime,
            "checksum": checksum
        }

        print(f"请求参数: DeviceID={DEVICE_ID}, Time={curtime}")
        
        try:
            response = requests.post(AUTH_URL, headers=headers, json=payload)
            
            # 🔍 [DEBUG] 打印鉴权接口返回的完整 JSON
            if response.status_code == 200:
                resp_json = response.json()
                print(f"\n{Colors.BOLD}[HTTP 响应 (Auth)]:{Colors.ENDC}")
                print(json.dumps(resp_json, ensure_ascii=False, indent=2))  # <--- 打印 JSON
                
                self.token = resp_json.get('token')
                print(f"{Colors.GREEN}✅ 鉴权成功!{Colors.ENDC}")
                return True
            else:
                print(f"{Colors.FAIL}❌ 鉴权失败: {response.text}{Colors.ENDC}")
                return False
        except Exception as e:
            print(f"{Colors.FAIL}❌ 请求异常: {e}{Colors.ENDC}")
            return False

    def connect_ws(self):
        """
        第二步：建立 WebSocket 连接
        """
        if not self.token:
            print("请先获取 Token")
            return

        print(f"\n{Colors.HEADER}=== 2. 建立 WebSocket 连接 ==={Colors.ENDC}")

        # 构造 param 参数
        auth_data = json.dumps({"auth_id": DEVICE_ID})
        param = base64.b64encode(auth_data.encode('utf-8')).decode('utf-8')
        
        # 构造完整 URL
        url = f"{WS_URL}?param={param}&token={self.token}"
        print(f"连接地址: {WS_URL}...")

        self.ws = websocket.WebSocketApp(
            url,
            on_open=self.on_open,
            on_message=self.on_message,
            on_error=self.on_error,
            on_close=self.on_close
        )
        self.ws.run_forever()

    def on_open(self, ws):
        self.is_connected = True
        print(f"{Colors.GREEN}✅ WebSocket 连接成功!{Colors.ENDC}")
        threading.Thread(target=self.interaction_loop, daemon=True).start()

    def on_message(self, ws, message):
        """
        处理服务端返回的消息
        """
        try:
            msg_json = json.loads(message)
            
            # 🔍 [DEBUG] 打印 WebSocket 收到的每一条完整 JSON
            # 使用不同颜色区分，方便查看
            print(f"\n{Colors.BOLD}{Colors.CYAN}▼ [WebSocket 收到消息]:{Colors.ENDC}")
            print(json.dumps(msg_json, ensure_ascii=False, indent=2)) # <--- 打印 JSON

            # 解析业务数据 (依然保留解析逻辑，方便看重点)
            if 'data' in msg_json:
                data = msg_json['data']

                # 类型检查：防止 data 是字符串导致报错
                if not isinstance(data, dict):
                    return

                sub_type = data.get('sub')

                if sub_type == 'iat': # 识别结果
                    if data.get('is_last'):
                        print(f"{Colors.BLUE}👂 [识别结果]: {data.get('text')}{Colors.ENDC}")
                
                elif sub_type == 'nlu': # 语义理解
                    intent = data.get('intent', {})
                    if isinstance(intent, dict):
                        answer = intent.get('answer', {}).get('text')
                        if answer:
                            print(f"{Colors.GREEN}🤖 [AI 回答]: {answer}{Colors.ENDC}")

                elif sub_type == 'tts': # 语音合成音频
                    content = data.get('content')
                    # 这里的 content 太长了，不建议在这里重复打印，上面 full dump 已经有了
                    if content and isinstance(content, str):
                        print(f"{Colors.WARNING}🔊 [TTS 音频包收到]{Colors.ENDC}")

        except Exception as e:
            print(f"解析消息错误: {e}")

    def on_error(self, ws, error):
        print(f"{Colors.FAIL}❌ WebSocket Error: {error}{Colors.ENDC}")

    def on_close(self, ws, *args):
        self.is_connected = False
        print(f"{Colors.WARNING}⚠️ 连接断开{Colors.ENDC}")

    def interaction_loop(self):
        time.sleep(1)
        print("\n" + "="*30)
        print("输入文本并回车发送给 AI (输入 q 退出)")
        print("="*30 + "\n")

        while self.is_connected:
            text = input("请输入: ")
            if text.lower() == 'q':
                self.ws.close()
                break
            
            if text.strip():
                self.send_text_interaction(text)

    def send_text_interaction(self, text):
        # 1. 发送 Start 帧
        start_payload = {
            "action": "start",
            "params": {
                "data_type": "text",
                "features": ["nlu", "tts"],
                "tts_properties": {
                    "vcn": "x4_lingxiaoqi_oral",
                    "speed": 50,
                    "volume": 50
                },
                "nlu_properties": {
                    "sn": DEVICE_ID
                }
            }
        }
        
        # 🔍 [DEBUG] 打印发送的 JSON
        print(f"\n{Colors.BOLD}{Colors.BLUE}▲ [WebSocket 发送 Start]:{Colors.ENDC}")
        print(json.dumps(start_payload, ensure_ascii=False, indent=2))
        
        self.ws.send(json.dumps(start_payload))

        # 2. 发送文本数据 (二进制)
        self.ws.send(text.encode('utf-8'), opcode=websocket.ABNF.OPCODE_BINARY)
        print(f"▲ [发送文本二进制]: {text}")

        # 3. 发送 End 帧
        end_payload = {"action": "end"}
        
        # 🔍 [DEBUG] 打印发送的 JSON
        print(f"{Colors.BOLD}{Colors.BLUE}▲ [WebSocket 发送 End]:{Colors.ENDC}")
        print(json.dumps(end_payload, ensure_ascii=False, indent=2))
        
        self.ws.send(json.dumps(end_payload))

if __name__ == "__main__":
    client = XiaolingClient()
    if client.get_token():
        client.connect_ws()