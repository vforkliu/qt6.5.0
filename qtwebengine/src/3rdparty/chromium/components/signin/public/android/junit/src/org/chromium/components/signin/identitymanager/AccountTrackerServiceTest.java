// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.signin.identitymanager;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.ArgumentMatchers.notNull;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.accounts.Account;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.Spy;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.mockito.quality.Strictness;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.LooperMode;

import org.chromium.base.Promise;
import org.chromium.base.task.test.CustomShadowAsyncTask;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.JniMocker;
import org.chromium.components.signin.AccountManagerFacadeProvider;
import org.chromium.components.signin.AccountUtils;
import org.chromium.components.signin.base.CoreAccountInfo;
import org.chromium.components.signin.test.util.FakeAccountManagerFacade;

import java.util.List;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * Robolectric tests for {@link AccountTrackerService}.
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE, shadows = {CustomShadowAsyncTask.class})
@LooperMode(LooperMode.Mode.LEGACY)
public class AccountTrackerServiceTest {
    private static final long ACCOUNT_TRACKER_SERVICE_NATIVE = 10001L;
    private static final String ACCOUNT_EMAIL = "test@gmail.com";

    @Rule
    public final MockitoRule mMockitoRule = MockitoJUnit.rule().strictness(Strictness.STRICT_STUBS);

    @Rule
    public final JniMocker mocker = new JniMocker();

    @Spy
    private final FakeAccountManagerFacade mFakeAccountManagerFacade =
            new FakeAccountManagerFacade();

    @Mock
    private AccountTrackerService.Natives mNativeMock;

    @Mock
    private Runnable mRunnableMock;

    @Mock
    private AccountTrackerService.Observer mObserverMock;

    @Captor
    private ArgumentCaptor<String[]> mGaiaIdsCaptor;

    @Captor
    private ArgumentCaptor<String[]> mEmailsCaptor;

    @Captor
    private ArgumentCaptor<List<CoreAccountInfo>> mAccountInfosCaptor;

    private AccountTrackerService mService;

    @Before
    public void setUp() {
        AccountManagerFacadeProvider.setInstanceForTests(mFakeAccountManagerFacade);
        mocker.mock(AccountTrackerServiceJni.TEST_HOOKS, mNativeMock);
        mFakeAccountManagerFacade.addAccount(AccountUtils.createAccountFromName(ACCOUNT_EMAIL));
        mService = new AccountTrackerService(ACCOUNT_TRACKER_SERVICE_NATIVE);
    }

    @After
    public void tearDown() {
        AccountManagerFacadeProvider.resetInstanceForTests();
    }

    @Test
    public void testSeedAccountsIfNeededBeforeAccountsAreSeeded() {
        mService.seedAccountsIfNeeded(mRunnableMock);

        verify(mFakeAccountManagerFacade).addObserver(notNull());
        verify(mNativeMock)
                .seedAccountsInfo(eq(ACCOUNT_TRACKER_SERVICE_NATIVE), mGaiaIdsCaptor.capture(),
                        mEmailsCaptor.capture());
        Assert.assertArrayEquals(
                new String[] {mFakeAccountManagerFacade.getAccountGaiaId(ACCOUNT_EMAIL)},
                mGaiaIdsCaptor.getValue());
        Assert.assertArrayEquals(new String[] {ACCOUNT_EMAIL}, mEmailsCaptor.getValue());
        verify(mRunnableMock).run();
    }

    @Test
    public void testSeedAccountsIfNeededWhenSeedingIsInProgress() {
        final AtomicBoolean isInvoked = new AtomicBoolean(false);
        doAnswer(invocation -> {
            if (!isInvoked.getAndSet(true)) {
                mService.seedAccountsIfNeeded(mRunnableMock);
            }
            return toGaiaId(invocation.getArgument(0));
        })
                .when(mFakeAccountManagerFacade)
                .getAccountGaiaId(anyString());
        verify(mNativeMock, never()).seedAccountsInfo(anyLong(), any(), any());

        mService.seedAccountsIfNeeded(() -> {});

        verify(mNativeMock)
                .seedAccountsInfo(eq(ACCOUNT_TRACKER_SERVICE_NATIVE), mGaiaIdsCaptor.capture(),
                        mEmailsCaptor.capture());
        Assert.assertArrayEquals(
                new String[] {mFakeAccountManagerFacade.getAccountGaiaId(ACCOUNT_EMAIL)},
                mGaiaIdsCaptor.getValue());
        Assert.assertArrayEquals(new String[] {ACCOUNT_EMAIL}, mEmailsCaptor.getValue());
        verify(mRunnableMock).run();
    }

    @Test
    public void testSeedAccountsIfNeededAfterAccountsAreSeeded() {
        mService.seedAccountsIfNeeded(() -> {});

        mService.seedAccountsIfNeeded(mRunnableMock);

        verify(mFakeAccountManagerFacade).addObserver(notNull());
        // Accounts should be seeded only once
        verify(mNativeMock)
                .seedAccountsInfo(eq(ACCOUNT_TRACKER_SERVICE_NATIVE), mGaiaIdsCaptor.capture(),
                        mEmailsCaptor.capture());
        Assert.assertArrayEquals(
                new String[] {mFakeAccountManagerFacade.getAccountGaiaId(ACCOUNT_EMAIL)},
                mGaiaIdsCaptor.getValue());
        Assert.assertArrayEquals(new String[] {ACCOUNT_EMAIL}, mEmailsCaptor.getValue());
        verify(mRunnableMock).run();
    }

    @Test
    public void testAddingNewAccountTriggersSeedingAccounts() {
        mService.seedAccountsIfNeeded(() -> {});
        mService.addObserver(mObserverMock);
        final Account newAccount = AccountUtils.createAccountFromName("test2@gmail.com");
        verify(mNativeMock).seedAccountsInfo(eq(ACCOUNT_TRACKER_SERVICE_NATIVE), any(), any());

        mFakeAccountManagerFacade.addAccount(newAccount);

        verify(mNativeMock, times(2))
                .seedAccountsInfo(eq(ACCOUNT_TRACKER_SERVICE_NATIVE), any(), any());
        // Verify the observer is invoked with correct arguments
        verify(mObserverMock).onAccountsSeeded(mAccountInfosCaptor.capture(), eq(true));
        final CoreAccountInfo coreAccountInfo = CoreAccountInfo.createFromEmailAndGaiaId(
                ACCOUNT_EMAIL, mFakeAccountManagerFacade.getAccountGaiaId(ACCOUNT_EMAIL));
        final CoreAccountInfo newCoreAccountInfo = CoreAccountInfo.createFromEmailAndGaiaId(
                newAccount.name, mFakeAccountManagerFacade.getAccountGaiaId(newAccount.name));
        Assert.assertArrayEquals(new CoreAccountInfo[] {coreAccountInfo, newCoreAccountInfo},
                mAccountInfosCaptor.getValue().toArray(new CoreAccountInfo[0]));
    }

    /**
     * This test reproduces the bug crbug/1193890 caused by the race condition without the fix.
     */
    @Test
    public void testAddingAccountTriggersSeedingWhenAnotherSeedingIsInProgress() {
        final Account newAccount = AccountUtils.createAccountFromName("test2@gmail.com");
        final AtomicBoolean isNewAccountAdded = new AtomicBoolean(false);
        doAnswer(invocation -> {
            final String email = invocation.getArgument(0);
            if (ACCOUNT_EMAIL.equals(email) && !isNewAccountAdded.getAndSet(true)) {
                // Add the new account when the old account fetches the gaia ID to
                // simulate the race condition.
                mFakeAccountManagerFacade.addAccount(newAccount);
            }
            return toGaiaId(email);
        })
                .when(mFakeAccountManagerFacade)
                .getAccountGaiaId(anyString());
        verify(mNativeMock, never()).seedAccountsInfo(anyLong(), any(), any());

        mService.seedAccountsIfNeeded(() -> {});

        verify(mNativeMock, times(2))
                .seedAccountsInfo(eq(ACCOUNT_TRACKER_SERVICE_NATIVE), mGaiaIdsCaptor.capture(),
                        mEmailsCaptor.capture());
        Assert.assertArrayEquals(
                "seedAccountsInfo() should be invoked with the old account alone in the"
                        + " first call.",
                new String[] {toGaiaId(ACCOUNT_EMAIL)}, mGaiaIdsCaptor.getAllValues().get(0));
        Assert.assertArrayEquals(new String[] {ACCOUNT_EMAIL}, mEmailsCaptor.getAllValues().get(0));

        Assert.assertArrayEquals(
                "seedAccountsInfo() should be invoked with the old account and the new account"
                        + " together in the second call.",
                new String[] {toGaiaId(ACCOUNT_EMAIL), toGaiaId(newAccount.name)},
                mGaiaIdsCaptor.getAllValues().get(1));
        Assert.assertArrayEquals(
                new String[] {ACCOUNT_EMAIL, newAccount.name}, mEmailsCaptor.getAllValues().get(1));
    }

    @Test
    public void testSeedAccountsWhenGaiaIdIsNull() {
        // When gaia ID is null, seedAccounts() will be called recursively in the
        // current code, the test sets a limit number for this invocation artificially
        // by mocking AccountManagerFacade#getAccounts().
        when(mFakeAccountManagerFacade.getAccountGaiaId(anyString())).thenReturn(null);
        final int expectedNumberOfInvocations = 3;
        final AtomicInteger invocationCount = new AtomicInteger(0);
        // This will cause mock counts for getAccounts() method to be 1 greater than
        // expectedNumberOfInvocations
        final List<Account> accounts = mFakeAccountManagerFacade.getAccounts().getResult();
        doAnswer(invocationMock
                -> invocationCount.incrementAndGet() < expectedNumberOfInvocations
                        ? Promise.fulfilled(accounts)
                        : new Promise<>())
                .when(mFakeAccountManagerFacade)
                .getAccounts();

        mService.seedAccountsIfNeeded(mRunnableMock);

        verify(mFakeAccountManagerFacade).addObserver(notNull());
        // The fact that returned gaia ID is null will trigger the seeding again.
        verify(mFakeAccountManagerFacade, times(expectedNumberOfInvocations + 1)).getAccounts();
        verify(mNativeMock, never()).seedAccountsInfo(anyLong(), any(), any());
        verify(mRunnableMock, never()).run();
    }

    @Test
    public void testSeedAccountsWithObserverAttached() {
        mService.addObserver(mObserverMock);
        verify(mObserverMock, never()).onAccountsSeeded(any(), anyBoolean());

        mService.seedAccountsIfNeeded(() -> {});

        verify(mObserverMock).onAccountsSeeded(mAccountInfosCaptor.capture(), eq(false));
        final CoreAccountInfo account = CoreAccountInfo.createFromEmailAndGaiaId(
                ACCOUNT_EMAIL, mFakeAccountManagerFacade.getAccountGaiaId(ACCOUNT_EMAIL));
        Assert.assertArrayEquals(new CoreAccountInfo[] {account},
                mAccountInfosCaptor.getValue().toArray(new CoreAccountInfo[0]));
    }

    @Test
    public void testSeedAccountsWithObserverRemoved() {
        mService.addObserver(mObserverMock);
        mService.removeObserver(mObserverMock);

        mService.seedAccountsIfNeeded(() -> {});

        verify(mObserverMock, never()).onAccountsSeeded(any(), anyBoolean());
    }

    private static String toGaiaId(String email) {
        return "gaia-id-" + email.replace("@", "_at_");
    }
}
