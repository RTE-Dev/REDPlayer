//
//  RedSettingsViewController.m
//  RedPlayerDemo
//
//  Created by zijie on 2023/4/25.

//  Copyright © 2024 Xiaohongshu. All rights reserved.
//

#import "RedSettingsViewController.h"
#import "RedPlayerSettings.h"
#import "RedDemoPlayerViewController.h"
@import Masonry;

@interface RedSettingsViewController ()<UITableViewDelegate, UITableViewDataSource, UITextViewDelegate>
@property (nonatomic, strong) NSMutableArray *settingOptions;
@property (nonatomic, strong) UIButton *debugPlayBtn;
@property (nonatomic, strong) UITableView *tableView;
@property (nonatomic, strong) UILabel *copyrightLabel;
@end

@implementation RedSettingsViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = UIColor.systemBackgroundColor;
    self.title = @"Settings";
    self.settingOptions = [@[
            [@{@"sectionName": @"Render Type",
               @"options": @[[self renderTypeName:RenderTypeMetal],
                             [self renderTypeName:RenderTypeOpenGLES],
                             [self renderTypeName:RenderTypeSampleBuffer]],
               @"selectIndex":[NSIndexPath indexPathForRow:[RedPlayerSettings sharedInstance].renderType inSection:0],
               @"expand": @(YES)
             } mutableCopy],
            [@{@"sectionName": @"Decoding Method",
               @"options": @[@"Hardware Decoding", @"Software Decoding"],
               @"selectIndex":[NSIndexPath indexPathForRow:[RedPlayerSettings sharedInstance].softDecoder ? 1 : 0 inSection:1],
               @"expand": @(YES)
             } mutableCopy],
            [@{@"sectionName": @"HDR",
               @"options": @[@"Enabled", @"Disabled"],
               @"sub_options": @[@"(with SampleBufferDisplayLayer + Hardware Decoding)", @""],
               @"selectIndex":[NSIndexPath indexPathForRow:[RedPlayerSettings sharedInstance].hdrSwitch ? 0 : 1 inSection:2],
               @"expand": @(NO)
             } mutableCopy],
            [@{@"sectionName": @"Background Pause",
               @"options": @[@"Enabled", @"Disabled"],
               @"selectIndex":[NSIndexPath indexPathForRow:[RedPlayerSettings sharedInstance].bgdNotPause ? 1 : 0 inSection:3],
               @"expand": @(YES)
             } mutableCopy],
            [@{@"sectionName": @"Auto-preload Online Sample Video",
               @"options": @[@"Enabled", @"Disabled"],
               @"selectIndex":[NSIndexPath indexPathForRow:[RedPlayerSettings sharedInstance].autoPreload ? 0 : 1 inSection:4],
               @"expand": @(YES)
             } mutableCopy]
    ] mutableCopy];
    [self setupTableView];
}

- (void)setupTableView {
    // Create a table view and set its delegate and data source to self
    self.tableView = [[UITableView alloc] initWithFrame:self.view.bounds style:UITableViewStylePlain];
    self.tableView.delegate = self;
    self.tableView.dataSource = self;

    // Add the table view to the view controller's view
    [self.view addSubview:self.tableView];
    [self.tableView mas_makeConstraints:^(MASConstraintMaker *make) {
        make.top.equalTo(self.view).offset(10);
        make.left.mas_equalTo(10);
        make.right.mas_equalTo(-10);
        make.bottom.equalTo(self.view.mas_safeAreaLayoutGuideBottom).offset(-40);
    }];

}

- (NSString *)renderTypeName:(RenderType)renderType {
    switch (renderType) {
        case RenderTypeMetal:
            return @"Metal";
        case RenderTypeOpenGLES:
            return @"OpenGLES";
        case RenderTypeSampleBuffer:
            return @"SampleBufferDisplayLayer";
        default:
            return @"";
    }
}

#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return self.settingOptions.count;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return [self.settingOptions[section][@"expand"] boolValue] ? [self.settingOptions[section][@"options"] count] : 0;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"settingsCell"];
    if (!cell) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:@"settingsCell"];
    }
    cell.textLabel.textColor = UIColor.grayColor;
    cell.textLabel.text = self.settingOptions[indexPath.section][@"options"][indexPath.row];
    NSString *subTitle = self.settingOptions[indexPath.section][@"sub_options"][indexPath.row];
    if (subTitle.length > 0) {
        cell.detailTextLabel.text = subTitle;
        cell.detailTextLabel.textColor = UIColor.grayColor;
    } else {
        cell.detailTextLabel.text = nil;
    }
    cell.accessoryType = (indexPath.row == [self indexPathForRowInSection:indexPath.section]) ? UITableViewCellAccessoryCheckmark : UITableViewCellAccessoryNone;
    
    return cell;
}

#pragma mark - Table view delegate

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section {
    return 30;
}

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section {
    UIView *headerView = [[UIView alloc] initWithFrame:CGRectMake(0, 0, tableView.frame.size.width, 30)];
    headerView.backgroundColor = [UIColor colorWithRed:0.9 green:0.9 blue:0.9 alpha:0.5];
    UILabel *titleLabel = [[UILabel alloc] initWithFrame:CGRectMake(10, 0, tableView.frame.size.width - 20, 30)];
    titleLabel.textColor = [UIColor blackColor];
    titleLabel.text = self.settingOptions[section][@"sectionName"];
    [headerView addSubview:titleLabel];
    
    UITapGestureRecognizer *tapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(headerViewTapped:)];
    [headerView addGestureRecognizer:tapGesture];
    headerView.tag = section;
    
    return headerView;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    [self updateIndexPath:indexPath forSection:indexPath.section];
    [self.tableView reloadData];
}

#pragma mark - Private methods

- (void)headerViewTapped:(UITapGestureRecognizer *)gestureRecognizer {
    NSInteger section = gestureRecognizer.view.tag;
    BOOL isExpanded = [self.settingOptions[section][@"expand"] boolValue];
    [self.settingOptions[section] setValue:@(!isExpanded) forKey:@"expand"];
    [self.tableView reloadSections:[NSIndexSet indexSetWithIndex:section] withRowAnimation:UITableViewRowAnimationAutomatic];
}


- (NSInteger)indexPathForRowInSection:(NSInteger)section {
    return ((NSIndexPath *)(self.settingOptions[section][@"selectIndex"])).row;
}

- (void)updateIndexPath:(NSIndexPath *)indexPath forSection:(NSInteger)section {
    self.settingOptions[section][@"selectIndex"] = indexPath;
    if (section == 0) {
        [RedPlayerSettings sharedInstance].renderType = (RenderType)indexPath.row;
    } else if (section == 1) {
        [RedPlayerSettings sharedInstance].softDecoder = indexPath.row == 1;
    } else if (section == 2) {
        [RedPlayerSettings sharedInstance].hdrSwitch = indexPath.row == 0;
    } else if (section == 3) {
        [RedPlayerSettings sharedInstance].bgdNotPause = indexPath.row == 1;
    } else if (section == 4) {
        [RedPlayerSettings sharedInstance].autoPreload = indexPath.row == 0;
    }
}

@end
