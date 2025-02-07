/*
    Copyright 2025 Picovoice Inc.

    You may not use this file except in compliance with the license. A copy of the license is located in the "LICENSE"
    file accompanying this source.

    Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
    an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
    specific language governing permissions and limitations under the License.
*/

using System;

namespace Pv
{
    public class OrcaException : Exception
    {
        private readonly string[] _messageStack;

        public OrcaException() { }

        public OrcaException(string message) : base(message) { }

        public OrcaException(string message, string[] messageStack) : base(ModifyMessages(message, messageStack))
        {
            this._messageStack = messageStack;
        }

        public string[] MessageStack
        {
            get => _messageStack;
        }

        private static string ModifyMessages(string message, string[] messageStack)
        {
            string messageString = message;
            if (messageStack.Length > 0)
            {
                messageString += ":";
                for (int i = 0; i < messageStack.Length; i++)
                {
                    messageString += $"\n  [{i}] {messageStack[i]}";
                }
            }
            return messageString;
        }

    }

    public class OrcaMemoryException : OrcaException
    {
        public OrcaMemoryException() { }

        public OrcaMemoryException(string message) : base(message) { }

        public OrcaMemoryException(string message, string[] messageStack) : base(message, messageStack) { }
    }

    public class OrcaIOException : OrcaException
    {
        public OrcaIOException() { }

        public OrcaIOException(string message) : base(message) { }

        public OrcaIOException(string message, string[] messageStack) : base(message, messageStack) { }
    }

    public class OrcaInvalidArgumentException : OrcaException
    {
        public OrcaInvalidArgumentException() { }

        public OrcaInvalidArgumentException(string message) : base(message) { }

        public OrcaInvalidArgumentException(string message, string[] messageStack) : base(message, messageStack) { }
    }

    public class OrcaStopIterationException : OrcaException
    {
        public OrcaStopIterationException() { }

        public OrcaStopIterationException(string message) : base(message) { }

        public OrcaStopIterationException(string message, string[] messageStack) : base(message, messageStack) { }
    }

    public class OrcaKeyException : OrcaException
    {
        public OrcaKeyException() { }

        public OrcaKeyException(string message) : base(message) { }

        public OrcaKeyException(string message, string[] messageStack) : base(message, messageStack) { }
    }

    public class OrcaInvalidStateException : OrcaException
    {
        public OrcaInvalidStateException() { }

        public OrcaInvalidStateException(string message) : base(message) { }

        public OrcaInvalidStateException(string message, string[] messageStack) : base(message, messageStack) { }
    }

    public class OrcaRuntimeException : OrcaException
    {
        public OrcaRuntimeException() { }

        public OrcaRuntimeException(string message) : base(message) { }

        public OrcaRuntimeException(string message, string[] messageStack) : base(message, messageStack) { }
    }

    public class OrcaActivationException : OrcaException
    {
        public OrcaActivationException() { }

        public OrcaActivationException(string message) : base(message) { }

        public OrcaActivationException(string message, string[] messageStack) : base(message, messageStack) { }
    }

    public class OrcaActivationLimitException : OrcaException
    {
        public OrcaActivationLimitException() { }

        public OrcaActivationLimitException(string message) : base(message) { }

        public OrcaActivationLimitException(string message, string[] messageStack) : base(message, messageStack) { }
    }

    public class OrcaActivationThrottledException : OrcaException
    {
        public OrcaActivationThrottledException() { }

        public OrcaActivationThrottledException(string message) : base(message) { }

        public OrcaActivationThrottledException(string message, string[] messageStack) : base(message, messageStack) { }
    }

    public class OrcaActivationRefusedException : OrcaException
    {
        public OrcaActivationRefusedException() { }

        public OrcaActivationRefusedException(string message) : base(message) { }

        public OrcaActivationRefusedException(string message, string[] messageStack) : base(message, messageStack) { }
    }
}