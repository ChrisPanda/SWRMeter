/*
 *
 *
 *    CircularBuffer.h
 *
 *    Static circular buffer type.
 *
 *    License: GNU General Public License Version 3.0.
 *    
 *    Copyright (C) 2014 by Matthew K. Roberts, KK5JY. All rights reserved.
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *    
 *    This program is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *    
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see: http://www.gnu.org/licenses/
 *
 *
 */

#ifndef __CIRCULAR_BUFFER_H
#define __CIRCULAR_BUFFER_H

namespace KK5JY {
  namespace Collections {
    /// <summary>
    /// Circular buffer generic type.
    /// </summary>
    template <typename T>
    class CircularBuffer {
      private:
        /// <summary>
        /// The data storage.
        /// </summary>
        T *m_Data;

        /// <summary>
        /// Number of items in the buffer.
        /// </summary>
        int m_Count;

        /// <summary>
        /// The input index.
        /// </summary>
        int m_In;

        /// <summary>
        /// The output index.
        /// </summary>
        int m_Out;
        
        /// <summary>
        /// The buffer size.
        /// </summary>
        int m_Max;

      public:
        /// <summary>
        /// Gets the number of items in the <c>CircularBuffer</c>.
        /// </summary>
        int Count() const {
          return m_Count;
        }

        /// <summary>
        /// Returns true if the buffer is full.
        /// </summary>
        bool Full() const {
          return m_Count == m_Max;
        }

        /// <summary>
        /// Return the first item in the <c>CircularBuffer</c>.
        /// </summary>
        bool First(T &result) const {
          if (m_Count == 0) {
            return false;
          }

          result = m_Data[m_Out];
          return true;
        }

        /// <summary>
        /// Return the last item in the <c>CircularBuffer</c>.
        /// </summary>
        bool Last(T &result) const {
          if (m_Count == 0) {
            return false;
          }

          int idx = m_In - 1;
          if (idx == -1) {
            idx = m_Max - 1;
          }
          result = m_Data[idx];
          return true;
        }

      public:
        /// <summary>
        /// Create a new circular buffer.
        /// </summary>
        /// <param name="count">The number of items of storage to allocate.</param>
        CircularBuffer(int count) {
          m_Data = new T[count];
          m_Max = count;
          Clear();
        }

        /// <summary>
        /// Add a collection of items.
        /// </summary>
        void Add(T** items) {
          while (*items != 0) {
            Add(**items);
            items++;
          }
        }

        /// <summary>
        /// Add a new item.
        /// </summary>
        bool Add(const T &item) {
          if (m_Count == m_Max) {
            return false;
          }

          m_Data[m_In++] = item;
          m_In %= m_Max;
          ++m_Count;
          return true;
        }

        /// <summary>
        /// Remove and return an item.
        /// </summary>
        bool Remove(T &result) {
          if (m_Count == 0) {
            return false;
          }

          result = m_Data[m_Out++];
          m_Out %= m_Max;
          --m_Count;
          return true;
        }

        /// <summary>
        /// Remove items from the front of the buffer.
        /// </summary>
        /// <param name="count">The number of items to remove.</param>
        int RemoveItems(int count) {
          if (count >= m_Count) {
            count = m_Count;
            m_Count = 0;
            m_Out = m_In;
            return count;
          }

          m_Out = (m_Out + count) % m_Max;
          m_Count -= count;
          return count;
        }

        /// <summary>
        /// Clear the circular buffer.
        /// </summary>
        void Clear() {
          m_In = m_Out = m_Count = 0;
        }

        /// <summary>
        /// Get the specified item.
        /// </summary>
        /// <param name="index">The index of the item.</param>
        /// <returns>The item.</returns>
        T ItemAt(int index) {
          int offset = (m_Out + index) % m_Max;
          return m_Data[offset];
        }
    };
  }
}

#endif
