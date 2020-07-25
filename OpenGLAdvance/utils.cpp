#include <windows.h>
#include <stdio.h>
#include "utils.h"

static GLuint CompileShader(const int& shaderType, const char * const & shaderCode);

char* LoadFileContent(const char* const& filePath)
{
	if (filePath == nullptr)
	{
		printf("LoadFileContent():filePath is nullptr!\n");
		return nullptr;
	}
	FILE* pFile = nullptr;
	fopen_s(&pFile, filePath, "rb");
	if (pFile == nullptr)
	{
		printf("LoadFileContent():open file failed!\n");
		return nullptr;
	}

	fseek(pFile, 0, SEEK_END);
	int len = ftell(pFile);
	char* buffer = new char[len + 1];
	rewind(pFile);
	fread(buffer, len, 1, pFile);
	buffer[len] = '\0';
	fclose(pFile);

	//printf("%s\n", buffer);

	return buffer;
}

GLuint CompileShader(const int& shaderType, const char * const & shaderCode)
{
	if (shaderCode == nullptr)
	{
		printf("CompileShader():shaderCode is nullptr!\n");
		return 0;
	}
	GLuint shader = glCreateShader(shaderType);
	if (shader == 0)
	{
		glDeleteShader(shader);
		shader = 0;
		printf("CompileShader():glCreateShader failed!\n");
		return 0;
	}
	glShaderSource(shader, 1, &shaderCode, nullptr); //传入显卡
	glCompileShader(shader);
	GLint compileResult = GL_TRUE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileResult);
	if (compileResult != GL_TRUE)
	{
		char szLog[1024] = { 0 };
		GLsizei logLen = 0;
		glGetShaderInfoLog(shader, 1024, &logLen, szLog);
		printf("CompileShader():glCompileShader failed!\nlog\n:%s\ncode\n:%s\n", szLog, shaderCode);
		glDeleteShader(shader);
		shader = 0;
		return shader;
	}

	return shader;
}

GLuint CreateGPUProgram(const char * const & vsShaderCode, const char * const & fsShaderCode)
{
	GLuint vsShader = CompileShader(GL_VERTEX_SHADER,vsShaderCode);
	GLuint fsShader = CompileShader(GL_FRAGMENT_SHADER, fsShaderCode);

	GLuint program = glCreateProgram();
	glAttachShader(program, vsShader);
	glAttachShader(program, fsShader);
	glLinkProgram(program);
	glDetachShader(program, vsShader);
	glDetachShader(program, fsShader);
	glDeleteShader(vsShader);
	glDeleteShader(fsShader);

	GLint linkResult = GL_TRUE;
	glGetProgramiv(program, GL_LINK_STATUS, &linkResult);
	if (linkResult != GL_TRUE)
	{
		char szLog[1024] = { 0 };
		GLsizei logLen = 0;
		glGetProgramInfoLog(program, 1024, &logLen, szLog);
		printf("CreateGPUProgram():glLinkProgram failed!\nvs\n:%s\nfs\n:%s\n", vsShaderCode, fsShaderCode);
		glDeleteProgram(program);
		program = 0;
		return program;
	}

	return program;
}

GLuint CreateGPUBufferObject(GLenum targetType, GLsizeiptr size, GLenum usage, const void * data)
{
	GLuint object;
	glGenBuffers(1, &object);
	glBindBuffer(targetType, object);
	glBufferData(targetType, size, data, usage);
	glBindBuffer(targetType, 0);
	return object;
}

const unsigned long FROMAT_DXT1 = 0x31545844l;//DXT1 -> 1 T X D 的asci码
static unsigned char* DecodeDXT(const char* const& fileContent, int& width, int& height, int& size)
{
	height = *(unsigned long*)(fileContent + sizeof(unsigned long) * 3);
	width = *(unsigned long*)(fileContent + sizeof(unsigned long) * 4);
	size = *(unsigned long*)(fileContent + sizeof(unsigned long) * 5);
	unsigned long compressFormat = *(unsigned long*)(fileContent + sizeof(unsigned long) * 21);
	switch (compressFormat)
	{
	case FROMAT_DXT1:
		printf("DXT1\n");

		break;
	default:
		break;
	}

	unsigned char* pPixelData = new unsigned char[size];
	memcpy(pPixelData, (fileContent + sizeof(unsigned long) * 32), size);

	return pPixelData;
}

GLuint CreateTexture(const char* const& filePath)
{
	if (filePath == nullptr)
	{
		printf("CreateTexture(): filepath is nullpt!\n");
		return -1;
	}
	char* pFileContent = LoadFileContent(filePath);
	if (pFileContent == nullptr)
	{
		printf("CreateTexture(): pFileContent is nullpt!\n");
		return -1;
	}

	int width = 0;
	int height = 0;
	int dxt1size = 0;
	unsigned char* piexelData = nullptr;
	GLenum format = GL_RGB;
	if (*((unsigned short*)pFileContent) == 0x4D42)
	{
		piexelData = LoadBMP(filePath, width, height);
	}
	else if (memcmp(pFileContent, "DDS ", 4) == 0) //压缩格式,有alpha通道
	{
		piexelData = DecodeDXT(pFileContent, width, height, dxt1size);
		format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
	}
	if (nullptr == piexelData)
	{
		printf("CreateTexture(): decode data failed!\n");
		return -1;
	}

	GLuint textureId;
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D,textureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); //超过的变成1
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	if (format == GL_RGB)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, piexelData);
	}
	else if (format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT)
	{
		glCompressedTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, dxt1size, piexelData);
	}
	glBindTexture(GL_TEXTURE_2D, 0);

	delete piexelData;
	piexelData = nullptr;

	return textureId;
}

void SavePixelDataToBMP(
	const char* const& filePath,
	unsigned char* const& pixelData,
	const int& width,
	const int& height)
{
	FILE* pFile;
	fopen_s(&pFile, filePath, "wb");
	if (pFile)
	{
		BITMAPFILEHEADER bfh;
		memset(&bfh, 0, sizeof(BITMAPFILEHEADER));
		bfh.bfType = 0x4D42;
		bfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + width * height * 3;
		bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
		fwrite(&bfh, sizeof(BITMAPFILEHEADER), 1, pFile);

		BITMAPINFOHEADER bih;
		memset(&bih, 0, sizeof(BITMAPINFOHEADER));
		bih.biWidth = width;
		bih.biHeight = height;
		bih.biBitCount = 24;
		bih.biSize = sizeof(BITMAPINFOHEADER);
		fwrite(&bih, sizeof(BITMAPINFOHEADER), 1, pFile);

		for (int i = 0; i < width * height * 3; i += 3)
		{
			pixelData[i] = pixelData[i] ^ pixelData[i + 2];
			pixelData[i + 2] = pixelData[i] ^ pixelData[i + 2];
			pixelData[i] = pixelData[i] ^ pixelData[i + 2];
		}
		fwrite(pixelData, width * height * 3, 1, pFile);
	}

	fclose(pFile);
	return;
}

unsigned char* LoadBMP(const char* const& path, int& width, int& height)
{
	unsigned char* imageData = nullptr;
	FILE* pFile;
	fopen_s(&pFile, path, "rb");
	if (pFile)
	{
		BITMAPFILEHEADER bfh;
		fread(&bfh, sizeof(BITMAPFILEHEADER), 1, pFile);
		if (bfh.bfType == 0x4D42) //0x4D42:bmp
		{
			BITMAPINFOHEADER bih;
			fread(&bih, sizeof(BITMAPINFOHEADER), 1, pFile);
			width = bih.biWidth;
			height = bih.biHeight;
			int pixelCount = width * height * 3;
			imageData = new unsigned char[pixelCount];
			fseek(pFile, bfh.bfOffBits, SEEK_SET);
			fread(imageData, 1, pixelCount, pFile);

			unsigned char temp;
			for (int i = 0; i < pixelCount; i += 3)
			{
				imageData[i] = imageData[i] ^ imageData[i + 2];
				imageData[i + 2] = imageData[i] ^ imageData[i + 2];
				imageData[i] = imageData[i] ^ imageData[i + 2];
			}
		}
		fclose(pFile);
	}
	return imageData;
}
