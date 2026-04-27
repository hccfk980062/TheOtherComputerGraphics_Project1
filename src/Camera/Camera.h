//9999% BETTER THAN THAT USELESS ARCBALL (Comment in 2024/11)

#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// 攝影機移動方向列舉，抽象化視窗系統的輸入方法
enum Camera_Movement {
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT
};

// 攝影機預設參數值
const float YAW         = 0.0f;
const float PITCH       = 0.0f;
const float SPEED       = 2.5f;
const float SENSITIVITY = 0.1f;
const float ZOOM        = 45.0f;

// 以尤拉角（Yaw / Pitch）驅動的自由飛行攝影機
// 處理鍵盤平移、滑鼠旋轉、滾輪縮放，並計算 View / Projection 矩陣
class Camera
{
public:
	// ── 攝影機屬性 ────────────────────────────────────────────────────────
	glm::vec3 Position;  // 世界座標位置
	glm::vec3 Front;     // 攝影機朝向（單位向量）
	glm::vec3 Up;        // 攝影機上方向（由 Right × Front 決定）
	glm::vec3 Right;     // 攝影機右方向（由 Front × WorldUp 決定）
	glm::vec3 WorldUp;   // 世界上方向（正常為 (0,1,0)，倒置時為 (0,-1,0)）

	// ── 尤拉角 ───────────────────────────────────────────────────────────
	float Yaw;    // 水平旋轉角（度），0° 朝 +X 方向
	float Pitch;  // 垂直仰角（度），正值向上

	// ── 操作參數 ─────────────────────────────────────────────────────────
	float MovementSpeed;     // 鍵盤移動速度（world units/秒）
	float MouseSensitivity;  // 滑鼠靈敏度（旋轉角/像素）
	float Zoom;              // 視角（FoV，度），目前未實際用於 Perspective

	int windowWidth;   // 視口寬度（用於計算投影矩陣）
	int windowHeight;  // 視口高度

	// 向量版建構子
	Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH)
		: Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
	{
		Position = position;
		WorldUp  = up;
		Yaw      = yaw;
		Pitch    = pitch;
		updateCameraVectors();
	}

	// 純量版建構子
	Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch)
		: Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
	{
		Position = glm::vec3(posX, posY, posZ);
		WorldUp  = glm::vec3(upX, upY, upZ);
		Yaw      = yaw;
		Pitch    = pitch;
		updateCameraVectors();
	}

	// 以 LookAt 方式計算 View 矩陣（從 Position 朝 Position+Front 看）
	glm::mat4 GetViewMatrix()
	{
		return glm::lookAt(Position, Position + Front, Up);
	}

	// 設定視口解析度，供 GetProjectionMatrix 計算正確寬高比
	void SetProjectionMatrix(int width, int height)
	{
		windowWidth  = width;
		windowHeight = height;
	}

	// 45° FoV 透視投影，近剪裁面 0.1，遠剪裁面 100
	glm::mat4 GetProjectionMatrix()
	{
		return glm::perspective(glm::radians(45.0f), (float)windowWidth / windowHeight, 0.1f, 100.0f);
	}

	// 鍵盤移動：vec[0~5] 對應 W/S/A/D/Q/E，每幀依 deltaTime 移動
	void ProcessKeyboard(std::array<bool, 6> vec, float deltaTime)
	{
		float velocity = MovementSpeed * deltaTime;
		if (vec[0]) Position += Front    * velocity;  // W：前進
		if (vec[1]) Position -= Front    * velocity;  // S：後退
		if (vec[2]) Position -= Right    * velocity;  // A：左移
		if (vec[3]) Position += Right    * velocity;  // D：右移
		if (vec[4]) Position -= WorldUp  * velocity;  // Q：下降
		if (vec[5]) Position += WorldUp  * velocity;  // E：上升
		updateCameraVectors();
	}

	// 滑鼠旋轉：xoffset 控制 Yaw，yoffset 控制 Pitch
	// 俯仰超過 ±90° 時翻轉 WorldUp，模擬 Pitch 環繞效果
	void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true)
	{
		xoffset *= MouseSensitivity;
		yoffset *= MouseSensitivity;

		Yaw   += xoffset;
		Pitch += yoffset;

		if (constrainPitch)
		{
			// Pitch 超過 ±90° 時天空方向翻轉，使攝影機可以「翻滾」越過頭頂
			if ((Pitch >= 90.0f && Pitch <= 270.0f) || (Pitch <= -90.0f && Pitch >= -270.0f))
				WorldUp = glm::vec3(0, -1, 0);
			else
				WorldUp = glm::vec3(0, 1, 0);

			// 超過一整圈時重設，防止浮點數累積誤差
			if (Pitch >= 360.0f || Pitch <= -360.0f)
				Pitch = 0;
		}

		updateCameraVectors();
	}

	// 滾輪縮放：調整 Zoom（FoV），限制在 1°~45° 之間
	void ProcessMouseScroll(float yoffset)
	{
		Zoom -= (float)yoffset;
		if (Zoom < 1.0f)  Zoom = 1.0f;
		if (Zoom > 45.0f) Zoom = 45.0f;
	}

	// 由尤拉角重新計算 Front / Right / Up 三向量
	void updateCameraVectors()
	{
		glm::vec3 front;
		front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
		front.y = sin(glm::radians(Pitch));
		front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
		Front = glm::normalize(front);

		// Right = Front × WorldUp，Up = Right × Front（Gram-Schmidt 正規化）
		Right = glm::normalize(glm::cross(Front, WorldUp));
		Up    = glm::normalize(glm::cross(Right, Front));
	}

	// 直接指定 Front / Up 方向，並反推對應的 Yaw / Pitch 值
	// 用於場景初始化時設定攝影機朝向
	void configureLookAt(glm::vec3 frontVector, glm::vec3 upVector)
	{
		Front = glm::normalize(frontVector);
		Right = glm::normalize(glm::cross(Front, WorldUp));
		Up    = glm::normalize(upVector);

		// 從方向向量反推尤拉角，保持 ProcessMouseMovement 後的狀態一致性
		Yaw   = glm::degrees(atan2(frontVector.z, frontVector.x));
		Pitch = glm::degrees(asin(frontVector.y));

		if ((Pitch >= 90.0f && Pitch <= 270.0f) || (Pitch <= -90.0f && Pitch >= -270.0f))
			WorldUp = glm::vec3(0, -1, 0);
		else
			WorldUp = glm::vec3(0, 1, 0);
	}
};
#endif
