# -*- coding: utf-8 -*-
import datetime
import time
import argparse
import hashlib
import requests
import subprocess
import sys

parser = argparse.ArgumentParser()
parser.add_argument(
    "--username", "-n", type=str, default="zhouruibin", help="the username of owner"
)
parser.add_argument("--module", "-m", type=str,
                    required=True, help="the module name")
parser.add_argument("--branch", "-b", type=str,
                    default="master", help="the target branch name")
parser.add_argument("--version", "-v", type=str,
                    default="max", help="the target version")
parser.add_argument("--phase", "-p", type=str,
                    default="release", help="the phase of prod output,release or build")

class InnerToken:
    TOKEN_TYPE = "11"
    timestamp = ""

    def sign(self, appid, uid, sk):
        md5 = hashlib.md5()
        self.timestamp = str(time.mktime(
            datetime.datetime.now().timetuple())).split(".")[0]
        md5.update(f"{self.timestamp}{uid}{appid}{sk}".encode("utf-8"))
        # md5.update((%s%s%s%s % (self.timestamp, str(uid), str(appid), str(sk)).encode(utf-8))
        return md5.hexdigest()

    def generateToken(self, appid, uid, sk):
        sign = self.sign(appid, uid, sk)
        token = (
            self.TOKEN_TYPE + "." + sign + "." +
            self.timestamp + "." + str(uid) + "-" + str(appid)
        )
        return token


if __name__ == "__main__":
    args = parser.parse_args()
    if args.version == 'max':
        p = InnerToken()
        # http://hetu.baidu.com/api/personalCenter/myApplication#
        access_token = p.generateToken(
            25034723, 0, "NvKHmUQROTrbqhnm9KWkCO50Cs8LErZg")
        payload = {
            "access_token": access_token,
            "username": args.username,
            "module": args.module,
            "branch": args.branch,
        }
        if args.phase == "build":
                release_url = "http://inner.openapi.baidu.com/rest/2.0/agile/getBuildProd4Jpass"
                cmd = "prodHttpCmd"
                download_url = "prodHttpUrl"
        else:
                release_url = "http://inner.openapi.baidu.com/rest/2.0/agile/getReleaseMaxVersion"
                cmd = "outputHttpCmd"
                download_url = "outputHttpUrl"
        res = requests.get(release_url, params=payload)
        http_cmd = f'{res.json()[cmd]} -q'
        ret = subprocess.run(http_cmd, shell=True)
        if ret.returncode == 0:
            print(f'Succeeded downloaded {res.json()[download_url]}')
        else:
            print(f'Failed downloaded {res.json()[download_url]}')
    else:
        p = InnerToken()
        # http://hetu.baidu.com/api/personalCenter/myApplication#
        access_token = p.generateToken(
            25034723, 0, "NvKHmUQROTrbqhnm9KWkCO50Cs8LErZg")
        payload = {
            "access_token": access_token,
            "username": args.username,
            "module": args.module,
            "branch": args.branch,
            "version": args.version,
        }
        release_url = "http://inner.openapi.baidu.com/rest/2.0/agile/v1/releases/info-with-version"
        res = requests.get(release_url, params=payload)
        if res.json()["code"] != 200:
            print("Invalid version!")
            sys.exit(0)
        http_cmd = f'{res.json()["entities"]["wgetHttpCommand"]} -q'
        ret = subprocess.run(http_cmd, shell=True)
        if ret.returncode == 0:
            print(f'Succeeded downloaded {res.json()["entities"]["releaseOutputHttpUrl"]}')
        else:
            print(f'Failed downloaded {res.json()["entities"]["releaseOutputHttpUrl"]}')
        
