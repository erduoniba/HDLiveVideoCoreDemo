//
//  AntiqueVideoFilter.cpp
//  Pods
//
//  Created by Alex.Shi on 16/3/3.
//
//

#include "AntiqueVideoFilter.hpp"
#include <TargetConditionals.h>


#ifdef TARGET_OS_IPHONE

#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES3/gl.h>
#include <videocore/sources/iOS/GLESUtil.h>
#include <videocore/filters/FilterFactory.h>

#endif

namespace videocore { namespace filters {
    
    bool AntiqueVideoFilter::s_registered = AntiqueVideoFilter::registerFilter();
    
    bool
    AntiqueVideoFilter::registerFilter()
    {
        FilterFactory::_register("com.videocore.filters.antique", []() { return new AntiqueVideoFilter(); });
        return true;
    }
    
    AntiqueVideoFilter::AntiqueVideoFilter()
    : IVideoFilter(), m_initialized(false), m_bound(false)
    {
        
    }
    AntiqueVideoFilter::~AntiqueVideoFilter()
    {
        glDeleteProgram(m_program);
        glDeleteVertexArrays(1, &m_vao);
    }
    
    const char * const
    AntiqueVideoFilter::vertexKernel() const
    {
        
        KERNEL(GL_ES2_3, m_language,
               uniform mat4   position;
               attribute vec2 aPos;
               attribute vec2 texCoordinate;
               varying vec2   textureCoordinate;
               void main(void) {
                   gl_Position = position * vec4(aPos,0.,1.);
                   textureCoordinate = texCoordinate;
               }
               )
        
        return nullptr;
    }
    
    const char * const
    AntiqueVideoFilter::pixelKernel() const
    {
        KERNEL(GL_ES2_3, m_language,
               varying highp vec2 textureCoordinate;
               precision highp float;
               uniform sampler2D inputImageTexture;
               uniform sampler2D curve;
               void main()
               {
                   highp vec4 textureColor;
                   highp vec4 textureColorRes;
                   highp float satVal = 65.0 / 100.0;
                   
                   float xCoordinate = textureCoordinate.x;
                   float yCoordinate = textureCoordinate.y;
                   
                   highp float redCurveValue;
                   highp float greenCurveValue;
                   highp float blueCurveValue;
                   
                   textureColor = texture2D( inputImageTexture, vec2(xCoordinate, yCoordinate));
                   textureColorRes = textureColor;
                   
                   redCurveValue = texture2D(curve, vec2(textureColor.r, 0.0)).r;
                   greenCurveValue = texture2D(curve, vec2(textureColor.g, 0.0)).g;
                   blueCurveValue = texture2D(curve, vec2(textureColor.b, 0.0)).b;
                   
                   
                   highp float G = (redCurveValue + greenCurveValue + blueCurveValue);
                   G = G / 3.0;
                   
                   redCurveValue = ((1.0 - satVal) * G + satVal * redCurveValue);
                   greenCurveValue = ((1.0 - satVal) * G + satVal * greenCurveValue);
                   blueCurveValue = ((1.0 - satVal) * G + satVal * blueCurveValue);
                   
                   redCurveValue = (((redCurveValue) > (1.0)) ? (1.0) : (((redCurveValue) < (0.0)) ? (0.0) : (redCurveValue)));
                   greenCurveValue = (((greenCurveValue) > (1.0)) ? (1.0) : (((greenCurveValue) < (0.0)) ? (0.0) : (greenCurveValue)));
                   blueCurveValue = (((blueCurveValue) > (1.0)) ? (1.0) : (((blueCurveValue) < (0.0)) ? (0.0) : (blueCurveValue)));

                   redCurveValue = texture2D(curve, vec2(redCurveValue, 0.0)).a;
                   greenCurveValue = texture2D(curve, vec2(greenCurveValue, 0.0)).a;
                   blueCurveValue = texture2D(curve, vec2(blueCurveValue, 0.0)).a;

                   highp vec4 base = vec4(redCurveValue, greenCurveValue, blueCurveValue, 1.0);
                   highp vec4 overlayer = vec4(250.0/255.0, 227.0/255.0, 193.0/255.0, 1.0);
                   
                   textureColor = overlayer * base;
                   base = (textureColor - base) * 0.850980 + base;
                   textureColor = base; 
                   
                   gl_FragColor = vec4(textureColor.r, textureColor.g, textureColor.b, 1.0);
               }
        )
        
        return nullptr;
    }
    void
    AntiqueVideoFilter::initialize()
    {
        switch(m_language) {
            case GL_ES2_3:
            case GL_2: {
                setProgram(build_program(vertexKernel(), pixelKernel()));
                glGenVertexArrays(1, &m_vao);
                glBindVertexArray(m_vao);
                m_uMatrix = glGetUniformLocation(m_program, "position");
                mToneCurveTextureUniformLocation  = glGetUniformLocation(m_program, "curve");
                int attrpos = glGetAttribLocation(m_program, "aPos");
                int attrtex = glGetAttribLocation(m_program, "texCoordinate");
                int unitex = glGetUniformLocation(m_program, "inputImageTexture");
                
                glUniform1i(unitex, 0);
                
                glEnableVertexAttribArray(attrpos);
                glEnableVertexAttribArray(attrtex);
                
                glVertexAttribPointer(attrpos, BUFFER_SIZE_POSITION, GL_FLOAT, GL_FALSE, BUFFER_STRIDE, BUFFER_OFFSET_POSITION);
                glVertexAttribPointer(attrtex, BUFFER_SIZE_POSITION, GL_FLOAT, GL_FALSE, BUFFER_STRIDE, BUFFER_OFFSET_TEXTURE);
                m_initialized = true;
            }
                break;
            case GL_3:
                break;
        }
    }
    void
    AntiqueVideoFilter::bind()
    {
        switch(m_language) {
            case GL_ES2_3:
            case GL_2:
                if(!m_bound) {
                    if(!initialized()) {
                        initialize();
                    }
                    glUseProgram(m_program);
                    glBindVertexArray(m_vao);
                }
                glUniformMatrix4fv(m_uMatrix, 1, GL_FALSE, &m_matrix[0][0]);
                glUniform1i(mToneCurveTextureUniformLocation, 0);
                break;
            case GL_3:
                break;
        }
    }
    void
    AntiqueVideoFilter::unbind()
    {
        m_bound = false;
    }
}
}