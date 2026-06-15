let disabled = false;

window.addEventListener('load', function () {
  document.getElementById('quickLoginBtn').addEventListener('click', function () {
    if (!disabled) {
      if (document.getElementById('privacyCheckbox').checked) {
        window.ClientAPI.navigateToNative('OneKeyLogin', function (errCode, result) {
          console.log('navigateToNative callback: errCode=' + errCode + ' result=' + result);
        });
      } else {
        showToast();
      }
    }
  });

  document.getElementById('otherLoginBtn').addEventListener('click', function () {
    if (!disabled) {
      window.ClientAPI.showMsg('切换到其他登录方式', 'other_login');
    }
  });

  document.getElementById('backButton').addEventListener('click', function () {
    window.ClientAPI.showMsg('返回上一页', 'back');
  });

  document.getElementById('privacyCheckbox').addEventListener('change', function (e) {
    console.log('privacy checkbox changed: ' + e.target.checked);
  });
});

function showToast() {
  document.getElementById('agreementToast').classList.remove('hide');
  document.getElementById('agreementToast').classList.add('show');
}

function hideToast() {
  document.getElementById('agreementToast').classList.add('hide');
  document.getElementById('agreementToast').classList.remove('show');
}

function agreeAndLogin() {
  document.getElementById('privacyCheckbox').checked = true;
  hideToast();
  document.getElementById('quickLoginBtn').click();
}
