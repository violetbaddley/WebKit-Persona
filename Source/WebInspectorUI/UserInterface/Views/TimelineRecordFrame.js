/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

WebInspector.TimelineRecordFrame = function(graphDataSource, record)
{
    // FIXME: Convert this to a WebInspector.Object subclass, and call super().
    // WebInspector.Object.call(this);

    this._element = document.createElement("div");
    this._element.classList.add("timeline-record-frame");

    this._graphDataSource = graphDataSource;
    this._record = record || null;
};

// FIXME: Move to a WebInspector.Object subclass and we can remove this.
WebInspector.Object.deprecatedAddConstructorFunctions(WebInspector.TimelineRecordFrame);

WebInspector.TimelineRecordFrame.MaximumWidthPixels = 14;
WebInspector.TimelineRecordFrame.MinimumWidthPixels = 4;

WebInspector.TimelineRecordFrame.prototype = {
    constructor: WebInspector.TimelineRecordFrame,
    __proto__: WebInspector.Object.prototype,

    // Public

    get element()
    {
        return this._element;
    },

    get record()
    {
        return this._record;
    },

    set record(record)
    {
        this._record = record;
    },

    refresh(graphDataSource)
    {
        if (!this._record)
            return false;

        var frameIndex = this._record.frameIndex;
        var graphStartFrameIndex = Math.floor(graphDataSource.startTime);
        var graphEndFrameIndex = graphDataSource.endTime;

        // If this frame is completely before or after the bounds of the graph, return early.
        if (frameIndex < graphStartFrameIndex || frameIndex > graphEndFrameIndex)
            return false;

        this._element.style.width = (1 / graphDataSource.timelineOverview.secondsPerPixel) + "px";

        var graphDuration = graphDataSource.endTime - graphDataSource.startTime
        var recordLeftPosition = (frameIndex - graphDataSource.startTime) / graphDuration;
        this._updateElementPosition(this._element, recordLeftPosition, "left");
        this._updateChildElements(graphDataSource);

        return true;
    },

    // Private

    _updateChildElements(graphDataSource)
    {
        this._element.removeChildren();

        console.assert(this._record);
        if (!this._record)
            return;

        if (graphDataSource.graphHeightSeconds === 0)
            return;

        var frameElement = document.createElement("div");
        frameElement.classList.add("frame");
        this._element.appendChild(frameElement);

        var frameHeight = this._record.duration / graphDataSource.graphHeightSeconds;
        this._updateElementPosition(frameElement, frameHeight, "height");

        function createDurationElement(duration, taskType)
        {
            var element = document.createElement("div");
            this._updateElementPosition(element, duration / this._record.duration, "height");
            element.classList.add("duration", taskType);
            return element;
        }

        Object.keys(WebInspector.RenderingFrameTimelineRecord.TaskType).forEach(function(key) {
            var taskType = WebInspector.RenderingFrameTimelineRecord.TaskType[key];
            var duration = this._record.durationForTask(taskType);
            if (duration === 0)
                return;
            frameElement.insertBefore(createDurationElement.call(this, duration, taskType), frameElement.firstChild);
        }, this);
    },

    _updateElementPosition(element, newPosition, property)
    {
        newPosition *= 100;
        newPosition = newPosition.toFixed(2);

        var currentPosition = parseFloat(element.style[property]).toFixed(2);
        if (currentPosition !== newPosition)
            element.style[property] = newPosition + "%";
    }
};
