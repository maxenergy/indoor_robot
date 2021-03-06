#include "T3Face.hpp"

namespace interface
{
T3Face::T3Face(QObject* parent):QThread(parent)
{
    _faceBeReady = false;
    _frameIndex = 0;

}
T3Face* T3Face::t3face = new T3Face();

T3Face* T3Face::getInstance()
{
  //t3face->initT3Face();
  return t3face;
}
void T3Face::initT3Face()
{

    if(_faceBeReady)
    {
      return ;
    }
    _userInfoDatabase = new T3UserInfoDatabase();
    _userInfoDatabase->loadFaceDatabase(_userInfoLists);
    initFaceEngine();
    _faceBeReady = true;
}
T3FaceIdenIdenCode T3Face::initFaceEngine()
{
    //分配工作空间
    int FTWorkMemSize_ = 20 * 1024 * 1024;
    _FTWorkMem = new unsigned char[FTWorkMemSize_];
    int FRWorkMemSize_ = 40 * 1024 * 1024;
    _FRWorkMem = new unsigned char[FRWorkMemSize_];
    int FGWorkMemSize_ = 30 * 1024 * 1024;
    _FGWorkMem = new unsigned char[FGWorkMemSize_];
    int FAWorkMemSize_ = 30 * 1024 * 1024;
    _FAWorkMem = new unsigned char[FAWorkMemSize_];

    _FTEngine = NULL;
    _FREngine = NULL;
    _FGEngine = NULL;
    _FAEngine = NULL;

    int ret_ = AFT_FSDK_InitialFaceEngine((MPChar)kAPPID,
                                          (MPChar)kFT_SDKKEY,
                                          _FTWorkMem,
                                          FTWorkMemSize_,
                                          &_FTEngine,
                                          AFT_FSDK_OPF_0_HIGHER_EXT,
                                          16,
                                          kMaxFTFace);
    if(0 != ret_)
    {
        T3LOG <<"fail to AFT_FSDK_InitialFaceEngine():"<<ret_;
        free(_FTWorkMem);
        return FTEngineInitError;
    }

    ret_ = AFR_FSDK_InitialEngine((MPChar)kAPPID,
                                  (MPChar)kFR_SDKKEY,
                                  _FRWorkMem,
                                  FRWorkMemSize_,
                                  &_FREngine);
    if(0 != ret_)
    {
        T3LOG <<"fail to AFR_FSDK_InitialEngine():"<<ret_;
        free(_FRWorkMem);
        return FREngineInitError;
    }

    ret_ = ASGE_FSDK_InitGenderEngine(
          (MPChar)kAPPID,
          (MPChar)kFG_SDKKEY,
          _FGWorkMem,
          FGWorkMemSize_,
          &_FGEngine);
    if (0 != ret_)
    {
        T3LOG << stderr <<  "fail to ASGE_FSDK_InitGenderEngine():" << ret_;
        free(_FGWorkMem);
        return FGEngineInitError;
    }
    ret_ = ASAE_FSDK_InitAgeEngine((MPChar)kAPPID,
                                   (MPChar)kFA_SDKKEY,
                                   _FAWorkMem,
                                   FAWorkMemSize_,
                                   &_FAEngine);
    if (0 != ret_)
    {
        T3LOG << "fail to ASAE_FSDK_InitAgeEngine(): 0x%x\r\n" << ret_;
        free(_FAWorkMem);
        return FAEngineInitError;
    }
    T3LOG << "faceEngine init success";

}


T3FaceIdenIdenCode T3Face::detectFacePosition(QByteArray &frameData,
                                              model::T3FaceRecognInfo &faceRecognInfo)
{



    _frameData = &frameData;
    ASVLOFFSCREEN inputImg_ = { 0 };
    inputImg_.u32PixelArrayFormat = ASVL_PAF_YUYV;
    inputImg_.i32Width = kFrameWidth;
    inputImg_.i32Height = kFrameHeight;
    inputImg_.ppu8Plane[0] = (MUInt8 *)frameData.data();
    inputImg_.pi32Pitch[0] = inputImg_.i32Width * 2;
    int ret_ = AFT_FSDK_FaceFeatureDetect(_FTEngine,
                                       &inputImg_,
                                       &_faceResult);
    if(0 != ret_)
    {
      return DetechFaceError;

    }else
    {

        if(0 != _faceResult->nFace)
        {

          faceRecognInfo.setFaceNum(_faceResult->nFace);
          QList<model::T3FacePositionInfo> facePositionList_;
          for(int i = 0; i < _faceResult->nFace;i++)
          {
            model::T3FacePositionInfo facePosition_;
            facePosition_.setbottom(_faceResult->rcFace[i].bottom);
            facePosition_.setLeft(_faceResult->rcFace[i].left);
            facePosition_.setRight(_faceResult->rcFace[i].right);
            facePosition_.setTop(_faceResult->rcFace[i].top);
            facePositionList_.append(facePosition_);
            faceRecognInfo.setFacePositionList(facePositionList_);
          }
          return DetechFace;

        }else
        {
          return NotDetechFace;

        }


    }

}
T3FaceIdenIdenCode T3Face::facePairMatch(QByteArray &frameData,
                                         model::T3FaceRecognInfo &faceRecognInfo)

{



    _faceResult->nFace = faceRecognInfo.getFaceNum();
    QList<model::T3UserInfo> userInfos_;
    for(int i = 0; i < faceRecognInfo.getFaceNum(); i++)
    {

        model::T3FacePositionInfo position_ = faceRecognInfo.getFacePositionList().at(i);
        _faceResult->rcFace[i].bottom = position_.getBottom();
        _faceResult->rcFace[i].left = position_.getLeft();
        _faceResult->rcFace[i].right = position_.getRight();
        _faceResult->rcFace[i].top = position_.getTop();
        AFR_FSDK_FACEMODEL localFaceModels_ = { 0 };

        int ret_ = extractFRFeature(frameData,
                                   kFrameWidth,
                                   kFrameHeight,
                                   ASVL_PAF_YUYV,
                                   &_faceResult->rcFace[i],
                                   _faceResult->lfaceOrient,
                                   &localFaceModels_);
        if(0 == ret_)
        {
            float fMaxScore_ = 0.0f;
            int iMaxIndex_ = -1;
            int iMax2Index_ = -1;
            QListIterator<model::T3UserInfo> iterator_(_userInfoLists);
            model::T3UserInfo maxUserInfo_;
            while(iterator_.hasNext())
            {

                float fScore_ = 0.0f;
                model::T3UserInfo userInfo_ = iterator_.next();
                AFR_FSDK_FACEMODEL targetFaceModels_ = { 0 };
                targetFaceModels_.lFeatureSize = userInfo_.getFeature().size();
                targetFaceModels_.pbFeature = (unsigned char*)userInfo_.getFeature().data();
                ret_ = AFR_FSDK_FacePairMatching(_FREngine,
                                                &targetFaceModels_,
                                                &localFaceModels_,
                                                &fScore_);

                if(ret_ == 0)
                {
                    if(fScore_>fMaxScore_)
                    {
                        iMax2Index_ = iMaxIndex_;
                        iMaxIndex_ = userInfo_.getID();
                        maxUserInfo_ = userInfo_;
                        fMaxScore_ = fScore_;
                    }
                }
            }
            if((iMaxIndex_>=0)&&(fMaxScore_>kThreshold))
            { 
              userInfos_.append(maxUserInfo_);
            }else
            {
              model::T3UserInfo userInfo_;
              QByteArray feature_;
              feature_.resize(localFaceModels_.lFeatureSize);
              memcpy(feature_.data(),
                     localFaceModels_.pbFeature,
                     localFaceModels_.lFeatureSize);
              userInfo_.setFeature(feature_);
              userInfo_.setType(model::eGuest);
              userInfos_.append(userInfo_);
            }
        }


    }
    faceRecognInfo.setUserInfoList(userInfos_);

}

T3FaceIdenIdenCode T3Face::genderEstimatiom()
{

}
T3FaceIdenIdenCode T3Face::ageEstimation()
{

}

int T3Face::extractFRFeature(QByteArray &frameData,
                             int frameWidth,
                             int frameHeight,
                             int frameFormat,
                             MRECT *rect,
                             int faceOrient,
                             AFR_FSDK_FACEMODEL *faceModels)
{
  ASVLOFFSCREEN inputImg_ = { 0 };
  inputImg_.u32PixelArrayFormat = frameFormat;
  inputImg_.i32Width = frameWidth;
  inputImg_.i32Height = frameHeight;
  inputImg_.ppu8Plane[0] = (MUInt8 *)frameData.data();
  inputImg_.pi32Pitch[0] = inputImg_.i32Width * 2;

  AFR_FSDK_FACEINPUT faceInput_;
  faceInput_.lOrient = faceOrient;
  faceInput_.rcFace.left = rect[0].left;
  faceInput_.rcFace.top = rect[0].top;
  faceInput_.rcFace.right = rect[0].right;
  faceInput_.rcFace.bottom = rect[0].bottom;

  int ret_ = AFR_FSDK_ExtractFRFeature(_FREngine,
                                       &inputImg_,
                                       &faceInput_,
                                       faceModels);
  return ret_;
}

void T3Face::addNewUserInfo(const QImage &image,
                            model::T3UserInfo &userInfo)
{
  int height_ = image.height();
  int width_ = image.width();

  unsigned char *frameData_ = new unsigned char [height_*width_*3];
  int frameFromat_ = ASVL_PAF_RGB24_B8G8R8;
  int pixelIndex_ = 0;
  for(int i =0 ;i<width_;i++)
  {
      for(int j = 0;j<height_;j++)
      {
          QRgb rgb_ = image.pixel(i,j);
          *(frameData_ + pixelIndex_) = QColor(rgb_).blue();
          *(frameData_ + pixelIndex_ + 1) = QColor(rgb_).green();
          *(frameData_ + pixelIndex_ + 2) = QColor(rgb_).red();
          pixelIndex_ += 3;
      }
  }
  ASVLOFFSCREEN inputImg_ = { 0 };
  inputImg_.i32Height = width_;
  inputImg_.i32Width = height_;
  inputImg_.pi32Pitch[0] = height_*3;
  inputImg_.ppu8Plane[0] = frameData_;
  inputImg_.u32PixelArrayFormat = frameFromat_;
  T3LOG << inputImg_.i32Height;
  LPAFT_FSDK_FACERES faceResult_ = NULL;
  AFT_FSDK_FaceFeatureDetect(_FTEngine,
                             &inputImg_,
                             &faceResult_);
  T3LOG << faceResult_->nFace;
  if(faceResult_->nFace == 1)
  {
      T3LOG << "detect the faceFeature";
  }else
  {
      T3LOG << "notdetect the faceFeature";
  }
  AFR_FSDK_FACEINPUT faceInput_;
  faceInput_.lOrient = faceResult_->lfaceOrient;

  faceInput_.rcFace.left = faceResult_->rcFace->left;
  faceInput_.rcFace.bottom = faceResult_->rcFace->bottom;
  faceInput_.rcFace.right = faceResult_->rcFace->right;
  faceInput_.rcFace.top = faceResult_->rcFace->top;
  AFR_FSDK_FACEMODEL faceModels_;
   T3LOG << AFR_FSDK_ExtractFRFeature(_FREngine,
                                     &inputImg_,
                                     &faceInput_,
                                     &faceModels_);
  T3LOG << faceModels_.lFeatureSize;
  QByteArray feature_ ;
  feature_.resize(faceModels_.lFeatureSize);
  memcpy(feature_.data(),
         faceModels_.pbFeature,
         faceModels_.lFeatureSize);
  userInfo.setFeature(feature_);
  _userInfoDatabase->addNewFaceInfo(userInfo);


}

T3Face::~T3Face()
{
    delete _frameData;

  if (_FTEngine) {
      AFT_FSDK_UninitialFaceEngine(_FTEngine);
  }

  if (_FREngine) {
      AFR_FSDK_UninitialEngine(_FREngine);
  }
  if (_FGEngine) {
      ASGE_FSDK_UninitGenderEngine(_FGEngine);
  }
  if(_FAEngine)  {
      ASAE_FSDK_UninitAgeEngine(_FAEngine);
  }

  if(_FTWorkMem){
      delete[] _FTWorkMem;
      _FTWorkMem = NULL;
  }

  if(_FRWorkMem){
      delete[] _FRWorkMem;
      _FRWorkMem = NULL;
  }
  if(_FGWorkMem){
      delete[] _FGWorkMem;
      _FGWorkMem = NULL;
  }
  if(_FAWorkMem){
      delete[] _FAWorkMem;
      _FAWorkMem = NULL;
  }
}


}
