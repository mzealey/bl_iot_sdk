/**
  ******************************************************************************
  * @file    bl602_ring_buffer.c
  * @version V1.0
  * @date
  * @brief   This file is the common component c file
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2019 Bouffalo Lab</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of Bouffalo Lab nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

#include "ring_buffer.h"

/** @addtogroup  BL_Common_Component
 *  @{
 */

/** @addtogroup  RING_BUFFER
 *  @{
 */

/** @defgroup  RING_BUFFER_Private_Macros
 *  @{
 */

/*@} end of group RING_BUFFER_Private_Macros */

/** @defgroup  RING_BUFFER_Private_Types
 *  @{
 */

/*@} end of group RING_BUFFER_Private_Types */

/** @defgroup  RING_BUFFER_Private_Fun_Declaration
 *  @{
 */

/*@} end of group RING_BUFFER_Private_Fun_Declaration */

/** @defgroup  RING_BUFFER_Private_Variables
 *  @{
 */

/*@} end of group RING_BUFFER_Private_Variables */

/** @defgroup  RING_BUFFER_Global_Variables
 *  @{
 */

/*@} end of group RING_BUFFER_Global_Variables */

/** @defgroup  RING_BUFFER_Private_Functions
 *  @{
 */

/*@} end of group RING_BUFFER_Private_Functions */

/** @defgroup  RING_BUFFER_Public_Functions
 *  @{
 */

/****************************************************************************//**
 * @brief  Ring buffer init function
 *
 * @param  rbType: Ring buffer type structure pointer
 * @param  buffer: Pointer of ring buffer
 * @param  size: Size of ring buffer
 * @param  lockCb: Ring buffer lock callback function pointer
 * @param  unlockCb: Ring buffer unlock callback function pointer
 *
 * @return SUCCESS
 *
*******************************************************************************/
BL_Err_Type Ring_Buffer_Init(Ring_Buffer_Type* rbType,uint8_t* buffer,uint16_t size,ringBuffer_Lock_Callback* lockCb,ringBuffer_Lock_Callback* unlockCb)
{
    /* Init ring buffer pointer */
    rbType->pointer = buffer;

    /* Init read/write mirror and index */
    rbType->readMirror = 0;
    rbType->readIndex = 0;
    rbType->writeMirror = 0;
    rbType->writeIndex = 0;

    /* Set ring buffer size */
    rbType->size = size;

    /* Set lock and unlock callback function */
    rbType->lock = lockCb;
    rbType->unlock = unlockCb;

    return SUCCESS;
}


/****************************************************************************//**
 * @brief  Ring buffer reset function
 *
 * @param  rbType: Ring buffer type structure pointer
 *
 * @return SUCCESS
 *
*******************************************************************************/
BL_Err_Type Ring_Buffer_Reset(Ring_Buffer_Type* rbType)
{
    /* Clear read/write mirror and index */
    rbType->readMirror = 0;
    rbType->readIndex = 0;
    rbType->writeMirror = 0;
    rbType->writeIndex = 0;

    return SUCCESS;
}


/****************************************************************************//**
 * @brief  Use callback function to write ring buffer function
 *
 * @param  rbType: Ring buffer type structure pointer
 * @param  length: Length of data want to write
 * @param  writeCb: Callback function pointer
 * @param  parameter: Parameter that callback function may use
 *
 * @return Length of data actually write
 *
*******************************************************************************/
uint16_t Ring_Buffer_Write_Callback(Ring_Buffer_Type* rbType,uint16_t length,ringBuffer_Write_Callback* writeCb,void* parameter)
{
    uint16_t sizeRemained = Ring_Buffer_Get_Empty_Length(rbType);

    if(writeCb == NULL){
        return 0;
    }

    if(rbType->lock != NULL){
        rbType->lock();
    }

    /* Ring buffer has no space for new data */
    if(sizeRemained == 0){
        if(rbType->unlock != NULL){
            rbType->unlock();
        }
        return 0;
    }

    /* Drop part of data when length out of space remained */
    if(length > sizeRemained){
        length = sizeRemained;
    }

    /* Get size of space remained in current mirror */
    sizeRemained = rbType->size - rbType->writeIndex;

    if(sizeRemained > length){
        /* Space remained is enough for data in current mirror */
        writeCb(parameter,&rbType->pointer[rbType->writeIndex],length);
        rbType->writeIndex += length;
    }else{
        /* Data is divided to two parts with different mirror */
        writeCb(parameter,&rbType->pointer[rbType->writeIndex],sizeRemained);
        writeCb(parameter,&rbType->pointer[0],length-sizeRemained);
        rbType->writeIndex = length-sizeRemained;
        rbType->writeMirror = ~rbType->writeMirror;
    }

    if(rbType->unlock != NULL){
        rbType->unlock();
    }
    return length;
}


/****************************************************************************//**
 * @brief  Copy data from data buffer to ring buffer function
 *
 * @param  parameter: Pointer to source pointer
 * @param  dest: Ring buffer to write
 * @param  length: Length of data to write
 *
 * @return None
 *
*******************************************************************************/
static void Ring_Buffer_Write_Copy(void* parameter,uint8_t* dest,uint16_t length)
{
    uint8_t **src = (uint8_t **)parameter;

    ARCH_MemCpy_Fast(dest,*src,length);
    *src += length;
}


/****************************************************************************//**
 * @brief  Write ring buffer function
 *
 * @param  rbType: Ring buffer type structure pointer
 * @param  data: Data to write
 * @param  length: Length of data
 *
 * @return Length of data writted actually
 *
*******************************************************************************/
uint16_t Ring_Buffer_Write(Ring_Buffer_Type* rbType,const uint8_t* data,uint16_t length)
{
    return Ring_Buffer_Write_Callback(rbType,length,Ring_Buffer_Write_Copy,&data);
}


/****************************************************************************//**
 * @brief  Write 1 byte to ring buffer function
 *
 * @param  rbType: Ring buffer type structure pointer
 * @param  data: Data to write
 *
 * @return Length of data writted actually
 *
*******************************************************************************/
uint16_t Ring_Buffer_Write_Byte(Ring_Buffer_Type* rbType,const uint8_t data)
{
    if(rbType->lock != NULL){
        rbType->lock();
    }

    /* Ring buffer has no space for new data */
    if(!Ring_Buffer_Get_Empty_Length(rbType)){
        if(rbType->unlock != NULL){
            rbType->unlock();
        }
        return 0;
    }

    rbType->pointer[rbType->writeIndex] = data;

    /* Judge to change index and mirror */
    if(rbType->writeIndex != (rbType->size-1)){
        rbType->writeIndex++;
    }else{
        rbType->writeIndex = 0;
        rbType->writeMirror = ~rbType->writeMirror;
    }

    if(rbType->unlock != NULL){
        rbType->unlock();
    }
    return 1;
}


/****************************************************************************//**
 * @brief  Write ring buffer function, old data will be covered by new data when ring buffer is
 *         full
 *
 * @param  rbType: Ring buffer type structure pointer
 * @param  data: Data to write
 * @param  length: Length of data
 *
 * @return Length of data writted actually
 *
*******************************************************************************/
uint16_t Ring_Buffer_Write_Force(Ring_Buffer_Type* rbType,const uint8_t* data,uint16_t length)
{
    uint16_t sizeRemained = Ring_Buffer_Get_Empty_Length(rbType);
    uint16_t indexRemained = rbType->size - rbType->writeIndex;

    if(rbType->lock != NULL){
        rbType->lock();
    }

    /* Drop extra data when data length is large than size of ring buffer */
    if(length > rbType->size){
        data = &data[length - rbType->size];
        length = rbType->size;
    }

    if(indexRemained > length){
        /* Space remained is enough for data in current mirror */
        ARCH_MemCpy_Fast(&rbType->pointer[rbType->writeIndex],data,length);
        rbType->writeIndex += length;

        /* Update read index */
        if(length > sizeRemained){
            rbType->readIndex = rbType->writeIndex;
        }
    }else{
        /* Data is divided to two parts with different mirror */
        ARCH_MemCpy_Fast(&rbType->pointer[rbType->writeIndex],data,indexRemained);
        ARCH_MemCpy_Fast(&rbType->pointer[0],&data[indexRemained],length-indexRemained);
        rbType->writeIndex = length-indexRemained;
        rbType->writeMirror = ~rbType->writeMirror;

        /* Update read index and mirror */
        if(length > sizeRemained){
            rbType->readIndex = rbType->writeIndex;
            rbType->readMirror = ~rbType->readMirror;
        }
    }

    if(rbType->unlock != NULL){
        rbType->unlock();
    }
    return length;
}


/****************************************************************************//**
 * @brief  Write 1 byte to ring buffer function, old data will be covered by new data when ring
 *         buffer is full
 *
 * @param  rbType: Ring buffer type structure pointer
 * @param  data: Data to write
 *
 * @return Length of data writted actually
 *
*******************************************************************************/
uint16_t Ring_Buffer_Write_Byte_Force(Ring_Buffer_Type* rbType,const uint8_t data)
{
    Ring_Buffer_Status_Type status = Ring_Buffer_Get_Status(rbType);

    if(rbType->lock != NULL){
        rbType->lock();
    }

    rbType->pointer[rbType->writeIndex] = data;

    /* Judge to change index and mirror */
    if(rbType->writeIndex == rbType->size-1){
        rbType->writeIndex = 0;
        rbType->writeMirror = ~rbType->writeMirror;

        /* Update read index and mirror */
        if(status == RING_BUFFER_FULL){
            rbType->readIndex = rbType->writeIndex;
            rbType->readMirror = ~rbType->readMirror;
        }
    }else{
        rbType->writeIndex++;

        /* Update read index */
        if(status == RING_BUFFER_FULL){
            rbType->readIndex = rbType->writeIndex;
        }
    }

    if(rbType->unlock != NULL){
        rbType->unlock();
    }
    return 1;
}


/****************************************************************************//**
 * @brief  Use callback function to read ring buffer function
 *
 * @param  rbType: Ring buffer type structure pointer
 * @param  length: Length of data want to read
 * @param  readCb: Callback function pointer
 * @param  parameter: Parameter that callback function may use
 *
 * @return Length of data actually read
 *
*******************************************************************************/
uint16_t Ring_Buffer_Read_Callback(Ring_Buffer_Type* rbType,uint16_t length,ringBuffer_Read_Callback* readCb,void* parameter)
{
    uint16_t size = Ring_Buffer_Get_Length(rbType);

    if(readCb == NULL){
        return 0;
    }

    if(rbType->lock != NULL){
        rbType->lock();
    }

    /* Ring buffer has no data */
    if(!size){
        if(rbType->unlock != NULL){
            rbType->unlock();
        }
        return 0;
    }

    /* Ring buffer do not have enough data */
    if(size < length){
        length = size;
    }

    /* Get size of space remained in current mirror */
    size = rbType->size - rbType->readIndex;

    if(size > length){
        /* Read all data needed */
        readCb(parameter,&rbType->pointer[rbType->readIndex],length);
        rbType->readIndex += length;
    }else{
        /* Read two part of data in different mirror */
        readCb(parameter,&rbType->pointer[rbType->readIndex],size);
        readCb(parameter,&rbType->pointer[0],length-size);
        rbType->readIndex = length-size;
        rbType->readMirror = ~rbType->readMirror;
    }

    if(rbType->unlock != NULL){
        rbType->unlock();
    }
    return length;
}


/****************************************************************************//**
 * @brief  Copy data from ring buffer to data buffer function
 *
 * @param  parameter: Pointer to destination pointer
 * @param  data: Data buffer to copy
 * @param  length: Length of data to copy
 *
 * @return None
 *
*******************************************************************************/
static void Ring_Buffer_Read_Copy(void* parameter,uint8_t* data,uint16_t length)
{
    uint8_t **dest = (uint8_t **)parameter;

    ARCH_MemCpy_Fast(*dest,data,length);
    *dest += length;
}


/****************************************************************************//**
 * @brief  Read ring buffer function
 *
 * @param  rbType: Ring buffer type structure pointer
 * @param  data: Buffer for data read
 * @param  length: Length of data to read
 *
 * @return Length of data read actually
 *
*******************************************************************************/
uint16_t Ring_Buffer_Read(Ring_Buffer_Type* rbType,uint8_t* data,uint16_t length)
{
    return Ring_Buffer_Read_Callback(rbType,length,Ring_Buffer_Read_Copy,&data);
}


/****************************************************************************//**
 * @brief  Read 1 byte from ring buffer function
 *
 * @param  rbType: Ring buffer type structure pointer
 * @param  data: Data read
 *
 * @return Length of data actually read
 *
*******************************************************************************/
uint16_t Ring_Buffer_Read_Byte(Ring_Buffer_Type* rbType,uint8_t* data)
{
    if(rbType->lock != NULL){
        rbType->lock();
    }

    /* Ring buffer has no data */
    if(!Ring_Buffer_Get_Length(rbType)){
        if(rbType->unlock != NULL){
            rbType->unlock();
        }
        return 0;
    }

    /* Read data */
    *data = rbType->pointer[rbType->readIndex];

    /* Update read index and mirror */
    if(rbType->readIndex == rbType->size-1){
        rbType->readIndex = 0;
        rbType->readMirror = ~rbType->readMirror;
    }else{
        rbType->readIndex++;
    }

    if(rbType->unlock != NULL){
        rbType->unlock();
    }
    return 1;
}


/****************************************************************************//**
 * @brief  Read ring buffer function, do not remove from buffer actually
 *
 * @param  rbType: Ring buffer type structure pointer
 * @param  data: Buffer for data read
 * @param  length: Length of data to read
 *
 * @return Length of data read actually
 *
*******************************************************************************/
uint16_t Ring_Buffer_Peek(Ring_Buffer_Type* rbType,uint8_t* data,uint16_t length)
{
    uint16_t size = Ring_Buffer_Get_Length(rbType);

    if(rbType->lock != NULL){
        rbType->lock();
    }

    /* Ring buffer has no data */
    if(!size){
        if(rbType->unlock != NULL){
            rbType->unlock();
        }
        return 0;
    }

    /* Ring buffer do not have enough data */
    if(size < length){
        length = size;
    }

    /* Get size of space remained in current mirror */
    size = rbType->size - rbType->readIndex;

    if(size > length){
        /* Read all data needed */
        ARCH_MemCpy_Fast(data,&rbType->pointer[rbType->readIndex],length);
    }else{
        /* Read two part of data in different mirror */
        ARCH_MemCpy_Fast(data,&rbType->pointer[rbType->readIndex],size);
        ARCH_MemCpy_Fast(&data[size],&rbType->pointer[0],length-size);
    }

    if(rbType->unlock != NULL){
        rbType->unlock();
    }
    return length;
}


/****************************************************************************//**
 * @brief  Read 1 byte from ring buffer function, do not remove from buffer actually
 *
 * @param  rbType: Ring buffer type structure pointer
 * @param  data: Data read
 *
 * @return Length of data actually read
 *
*******************************************************************************/
uint16_t Ring_Buffer_Peek_Byte(Ring_Buffer_Type* rbType,uint8_t* data)
{
    if(rbType->lock != NULL){
        rbType->lock();
    }

    /* Ring buffer has no data */
    if(!Ring_Buffer_Get_Length(rbType)){
        if(rbType->unlock != NULL){
            rbType->unlock();
        }
        return 0;
    }

    /* Read data */
    *data = rbType->pointer[rbType->readIndex];

    if(rbType->unlock != NULL){
        rbType->unlock();
    }
    return 1;
}


/****************************************************************************//**
 * @brief  Get length of data in ring buffer function
 *
 * @param  rbType: Ring buffer type structure pointer
 *
 * @return Length of data
 *
*******************************************************************************/
uint16_t Ring_Buffer_Get_Length(Ring_Buffer_Type* rbType)
{
    if(rbType->lock != NULL){
        rbType->lock();
    }

    if(rbType->readMirror == rbType->writeMirror){
        if(rbType->unlock != NULL){
            rbType->unlock();
        }
        return rbType->writeIndex - rbType->readIndex;
    }else{
        if(rbType->unlock != NULL){
            rbType->unlock();
        }
        return rbType->size - (rbType->readIndex - rbType->writeIndex);
    }
}


/****************************************************************************//**
 * @brief  Get space remained in ring buffer function
 *
 * @param  rbType: Ring buffer type structure pointer
 *
 * @return Length of space remained
 *
*******************************************************************************/
uint16_t Ring_Buffer_Get_Empty_Length(Ring_Buffer_Type* rbType)
{
    return (rbType->size - Ring_Buffer_Get_Length(rbType));
}


/****************************************************************************//**
 * @brief  Get ring buffer status function
 *
 * @param  rbType: Ring buffer type structure pointer
 *
 * @return Status of ring buffer
 *
*******************************************************************************/
Ring_Buffer_Status_Type Ring_Buffer_Get_Status(Ring_Buffer_Type* rbType)
{
    if(rbType->lock != NULL){
        rbType->lock();
    }

    /* Judge empty or full */
    if(rbType->readIndex == rbType->writeIndex){
        if(rbType->readMirror == rbType->writeMirror){
            if(rbType->unlock != NULL){
                rbType->unlock();
            }
            return RING_BUFFER_EMPTY;
        }else{
            if(rbType->unlock != NULL){
                rbType->unlock();
            }
            return RING_BUFFER_FULL;
        }
    }
    if(rbType->unlock != NULL){
        rbType->unlock();
    }
    return RING_BUFFER_PARTIAL;
}


/*@} end of group RING_BUFFER_Public_Functions */

/*@} end of group RING_BUFFER */

/*@} end of group BL_Common_Component */
