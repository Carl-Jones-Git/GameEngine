/*
 * Copyright (c) 2023 Carl Jones
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once
#include <d3d11_2.h>
#include <DirectXMath.h>

__declspec(align(16)) struct KeyFrame{
	DirectX::XMMATRIX mat;
	double rY;
	DirectX::XMVECTOR quart;
	double t;
};

class Animation
{
	double animStartT; // Game time when animation was started
	double animLengthT; //Total time for animation
	UINT startFrame;// first frame
	UINT endFrame;// last frame
	UINT currFrame; // current frame
	UINT nextFrame; // next frame

public:
	KeyFrame *	keyFrames;
	DirectX::XMMATRIX Animation::update(double seconds);
	Animation(int N, double time);
	~Animation();
};

