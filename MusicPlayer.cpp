#include "MusicPlayer.h"     // 当前类的头文件，包含了类的声明
#include "ui_MusicPlayer.h"  // 由 Qt Designer 自动生成的 UI 类头文件，包含了通过它设计的界面元素
#include <QFileDialog>       // 用于打开文件或文件夹选择对话框
#include <QDirIterator>      // 用于遍历指定目录中的文件和子目录

MusicPlayer::MusicPlayer(QWidget *parent)
    : QWidget(parent)           // 调用基类 QWidget 的构造函数
    , ui(new Ui::MusicPlayer)   // 创建 UI 管理对象
    , m_listModel(new QStandardItemModel(this))  // 创建音乐列表的数据模型，存储音乐项
    , m_mediaPlayer(new QMediaPlayer(this))      // 创建媒体播放器对象，用于播放音频和视频
    , m_audioOutput(new QAudioOutput(this))      // 创建音频输出对象，用于管理音频输出
    , currentSongIndex(-1)     // 初始没有播放任何歌曲
    , m_timer(new QTimer(this))
{
    ui->setupUi(this);
    ui->musicListView->setModel(m_listModel);    // 显示音乐列表
    ui->volumeBar->setVisible(false);
    ui->playBtn->setIcon(QIcon(":/Resource/play.png"));   // 初始设置为播放图标（即当前为未播放状态）
    m_mediaPlayer->setAudioOutput(m_audioOutput);

    // 获取音乐时长，当媒体播放器的播放时长发生变化时触发信号
    connect(m_mediaPlayer, &QMediaPlayer::durationChanged, this, [this](qint64 duration) {
        qInfo() << "Duration (ms):" << duration;
    });

    // 当播放状态变化即音乐播放结束时，自动切换到下一曲歌曲播放，并更新按钮图标和定时器
    connect(m_mediaPlayer, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState state) {
        updatePlayButtonIcon(state);

        if (state == QMediaPlayer::PlayingState) { // 播放时启动定时器，暂停/停止时停止定时器
            m_timer->start(1000); // 每秒更新一次（或更短间隔）
        } else {
            m_timer->stop();
        }

        if (state == QMediaPlayer::StoppedState) { // 自动切歌逻辑
            int currentIndex = getCurrentSongIndex();
            if (currentIndex == -1 || m_listModel->rowCount() == 0) {
                return;
            }
            int nextIndex = (currentIndex + 1) % m_listModel->rowCount();
            // 同步选中状态
            QModelIndex nextModelIndex = m_listModel->index(nextIndex, 0);
            ui->musicListView->setCurrentIndex(nextModelIndex);  // 高亮下一首
            on_nextBtn_clicked();
        }
    });

    // 连接音量进度条到音频输出，并连接时间进度条与两个时间标签
    connect(ui->volumeBar, &QSlider::valueChanged, this, &MusicPlayer::on_volumeBar_sliderMoved);
    connect(m_mediaPlayer, &QMediaPlayer::positionChanged, this, &MusicPlayer::updatePlayDurLab);
    connect(m_mediaPlayer, &QMediaPlayer::durationChanged, this, &MusicPlayer::updateDurationLab);
    connect(ui->playSlider, &QSlider::sliderMoved, this, &MusicPlayer::on_playSlider_sliderMoved);
}

MusicPlayer::~MusicPlayer()
{
    delete ui;
}

// 更新播放按钮图标
void MusicPlayer::updatePlayButtonIcon(QMediaPlayer::PlaybackState state)
{
    if (state == QMediaPlayer::PlayingState) {
        ui->playBtn->setIcon(QIcon(":/Resource/pause.png")); // 播放时显示暂停图标
    } else {
        ui->playBtn->setIcon(QIcon(":/Resource/play.png")); // 暂停/停止时显示播放图标
    }
}

void MusicPlayer::on_openDirBtn_clicked()
{
    // 打开文件夹选择对话框默认打开的目录路径
    auto path =  QFileDialog::getExistingDirectory(this, "选择文件夹", "E:/desktop/YXR/Audio_video_tech/QT/Qt_Projects/MusicPlayer/music");
    if (path.isEmpty()) {
        return;
    }

    // 打开要遍历的目录路径，读取并过滤出里面的所有 mp3、wav、ogg 和 flac 音乐文件
    QDirIterator it(path, {"*.mp3", "*.wav", "*.ogg", "*.flac"});
    while (it.hasNext()) { // 判断是否还有下一个匹配的文件，有则继续循环
        it.next(); // 移动到下一个文件，并加载它的信息
        QFileInfo info = it.fileInfo();
        auto item = new QStandardItem(info.fileName()); // 为每个音乐文件创建一个列表项，存入其文件名

        // 把音乐的完整路径存储起来，在音乐列表视图中显示对应文件名
        item->setData(it.fileInfo().canonicalFilePath(), Qt::UserRole + 1);
        m_listModel->appendRow(item);
    }
}

void MusicPlayer::on_musicListView_doubleClicked(const QModelIndex &index)
{
    auto filePath = index.data(Qt::UserRole + 1).toString();
    auto url = QUrl::fromLocalFile(filePath);

    m_mediaPlayer->setSource(url);
    m_mediaPlayer->play();
    ui->musicListView->setCurrentIndex(index);  // 选中当前播放的歌曲蓝色高亮
    qInfo() << "正在播放:" << url;

    currentSongIndex = index.row();
}

// 获取当前播放歌曲的索引
int MusicPlayer::getCurrentSongIndex() const
{
    if (currentSongIndex < 0 || currentSongIndex >= m_listModel->rowCount()) {
        return -1;
    }
    return currentSongIndex;
}

void MusicPlayer::on_prevBtn_clicked()
{
    int currentIndex = getCurrentSongIndex();
    if (currentIndex == -1 || m_listModel->rowCount() == 0) {
        return;  // 没有歌曲可播放
    }

    // 计算上一曲的索引，如果当前是第一首，则循环到最后一首
    int previousIndex = (currentIndex - 1 + m_listModel->rowCount()) % m_listModel->rowCount();

    // 获取上一曲的文件路径
    QModelIndex index = m_listModel->index(previousIndex, 0);
    QString filePath = index.data(Qt::UserRole + 1).toString();
    QUrl url = QUrl::fromLocalFile(filePath);

    // 设置新的源并播放
    m_mediaPlayer->setSource(url);
    m_mediaPlayer->play();

    // 更新当前播放索引，同步选中列表项，触发当前播放的歌曲蓝色高亮
    currentSongIndex = previousIndex;
    ui->musicListView->setCurrentIndex(index);

    qInfo() << "正在播放上一曲:" << url;
}

void MusicPlayer::on_nextBtn_clicked()
{
    int currentIndex = getCurrentSongIndex();
    if (currentIndex == -1 || m_listModel->rowCount() == 0) {
        return;
    }

    // 计算下一曲的索引，如果当前是最后一首，则循环到第一首
    int nextIndex = (currentIndex + 1) % m_listModel->rowCount();
    QModelIndex index = m_listModel->index(nextIndex, 0);
    QString filePath = index.data(Qt::UserRole + 1).toString();
    QUrl url = QUrl::fromLocalFile(filePath);
    m_mediaPlayer->setSource(url);
    m_mediaPlayer->play();

    currentSongIndex = nextIndex;
    ui->musicListView->setCurrentIndex(index);

    qInfo() << "正在播放下一曲:" << url;
}

void MusicPlayer::on_volumeBtn_clicked()
{
    bool visible = ui->volumeBar->isVisible();
    ui->volumeBar->setVisible(!visible);
}

qreal MusicPlayer::linearToLogVolume(int linearVolume)
{
    // 将 0-100 的线性值映射到 0.0-1.0 的对数音量
    qreal linear = linearVolume / 100.0;

    // 使用对数转换公式：logVolume = 10^(linearVolume/20)
    if (linear <= 0.0)
        return 0.0;
    else
        return qPow(10.0, (linear * 2.0) - 2.0); // 对应 -20dB 到 0dB
}

void MusicPlayer::on_volumeBar_sliderMoved(int value)
{
    qreal logVolume = linearToLogVolume(value);
    m_audioOutput->setVolume(logVolume);
}

void MusicPlayer::on_playBtn_clicked()
{
    if (m_listModel->rowCount() == 0 || currentSongIndex == -1) {
        return; // 若没有歌曲，不执行操作
    }

    // 直接根据当前媒体播放器状态切换
    if (m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
        m_mediaPlayer->pause();
    } else {
        m_mediaPlayer->play();
    }
}

void MusicPlayer::updateDurationLab(qint64 duration)
{
    QTime totalTime(0, 0);
    totalTime = totalTime.addMSecs(duration);
    ui->durationLab->setText(totalTime.toString("00:mm:ss")); // 总时长标签
    ui->playSlider->setRange(0, duration);
}

void MusicPlayer::updatePlayDurLab(qint64 position)
{
    QTime currentTime(0, 0);
    currentTime = currentTime.addMSecs(position);
    ui->playDurLab->setText(currentTime.toString("00:mm:ss")); // 已播放时长标签
    if (!ui->playSlider->isSliderDown()) { // 避免拖动时冲突
        ui->playSlider->setValue(position);
    }
}

void MusicPlayer::on_playSlider_sliderMoved(int position)
{
    m_mediaPlayer->setPosition(position);
}
