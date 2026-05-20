#os-release and arch info obtain
if [ -f /etc/redhat-release ];then
    if [ -f /opt/compiler/gcc-8.2/bin/g++ ];then
        os_release="bdcentos"
        version_id=""
    elif [ -f /opt/compiler/gcc-10/bin/g++ ];then
        os_release="bdcentos7"
        version_id=""
    else
        os_release=`rpm -q centos-release|cut -d- -f1`
        version_id=`rpm -q centos-release|cut -d- -f3|cut -d. -f1`
    fi
elif [ -f /etc/lsb-release ];then
    os_release=`awk -F= '/^DISTRIB_ID/{print $2}' /etc/lsb-release`
    version_id=`awk -F= '/^DISTRIB_RELEASE/{print $2}' /etc/lsb-release`
elif [ -f /etc/os-release ];then
    os_name=`awk -F= '/^NAME/{print $2}' /etc/os-release`
    if [[ $os_name =~ "Server" ]];then
        os_type="server_"
    fi
    os_release=`awk -F= '/^ID/{print $2}' /etc/os-release`
    version_id=`awk -F= '/^VERSION_ID/{print $2}' /etc/os-release`
fi
os_release=`echo ${os_release} | tr '[A-Z]' '[a-z]' | sed 's/\"//g'`
version_id=`echo ${version_id} | tr '[A-Z]' '[a-z]' | sed 's/[\"|\.]//g'`

#generate xtdk and xre output package name
if [[ $os_release == kylin ]];then
    xtdk_download="xtdk-llvm15-${os_release}${version_id}_${os_type}$(uname -m)" #TODO: xtdk output not support kylinv10_x86_64
    xre_download="xre-${os_release}_${version_id}-$(uname -m)"
    xccl_download="xccl_socket-${os_release}${version_id}_${os_type}$(uname -m)"
elif [[ $os_release == ubuntu ]];then
    xtdk_download="xtdk-llvm15-${os_release}${version_id}_${os_type}$(uname -m)" # ubuntu2004/ubuntu1604
    if [[ $version_id == "1804" ]];then
        xtdk_download="xtdk-llvm15-${os_release}1604_${os_type}$(uname -m)" # use ubuntu1604 output as before
    fi
    if [[ $DEVICE == KL3 ]];then
        xre_download="xre-${os_release}_${version_id}-$(uname -m)"
    else
        xre_download="xre-${os_release}_${version_id}_$(uname -m)"
    fi
    xccl_download="xccl_socket-${os_release}_${os_type}$(uname -m)"
elif [[ $os_release == bdcentos ]] || [[ $os_release == bdcentos7 ]];then
    xtdk_download="xtdk-llvm15-${os_release}${version_id}_$(uname -m)" # bdcentos
    if [[ $DEVICE == KL3 ]];then
        xre_download="xre-bdcentos-$(uname -m)" # xre bdcentos suite for 6u3 and 7u5
    else
        xre_download="xre-bdcentos_$(uname -m)" # xre bdcentos suite for 6u3 and 7u5
    fi
    xccl_download="xccl_socket-${os_release}_${os_type}$(uname -m)"
else
    xtdk_download="xtdk-llvm15-${os_release}${version_id}_$(uname -m)"
    xre_download="xre-${os_release}${version_id}_$(uname -m)"
    xccl_download="xccl_socket-${os_release}_${os_type}$(uname -m)"
fi

echo "current platform OS: ${os_release}"
echo "Required xtdk output: ${xtdk_download}"
echo "Required xre output: ${xre_download}"
echo "Required xccl output: ${xccl_download}"

export os_release=${os_release}
export xtdk_download=${xtdk_download}
export xre_download=${xre_download}
export xccl_download=${xccl_download}

echo "${os_release}"
echo "${xtdk_download}"
echo "${xre_download}"
echo "${xccl_download}"
