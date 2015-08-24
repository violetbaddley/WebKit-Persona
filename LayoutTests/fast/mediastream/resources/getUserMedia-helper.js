function getUserMedia(permission, constraints, successCallback, errorCallback) {
    if (window.testRunner)
        testRunner.setUserMediaPermission(permission == "allow");
    else {
        debug("This test can not be run without the testRunner");
        finishJSTest();
    }

    navigator.mediaDevices.getUserMedia(constraints).then(successCallback, reject).catch(defaultRejectOrCatch);

    function reject(e) {
        if (errorCallback)
            errorCallback(e);
        else
            defaultRejectOrCatch(e);
    }
}

function defaultRejectOrCatch(e) {
    testFailed('getUserMedia failed:' + e);
    finishJSTest();
}
